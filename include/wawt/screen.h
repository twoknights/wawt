/** @file screen.h
 *  @brief Base class for forms in the Wawt framework.
 *
 * Copyright 2018 Bruce Szablak
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WAWT_SCREEN_H
#define WAWT_SCREEN_H

#include "wawt/wawt.h"
#include "wawt/widgetfactory.h"

#include <any>
#include <cassert>
#include <chrono>
#include <experimental/type_traits>
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <utility>

namespace Wawt {

                            //=============
                            // class Screen
                            //=============

/**
 * @brief Exposes operational aspects of a user interface screen.
 *
 * The 'Screen' class provides an interface for managing user interface
 * elements (known as widgets), and integrating them into the event processing
 * model.  The user interface is presented in terms of a "screen" which is
 * composed from a sequence of rectangular regions.  These regions can be
 * drawn, and respond to "mouse" clicks.  The screen itself can be resized at
 * any point.
 *
 * Also, this class permits user interface elements to be dynamically added
 * to the end of screen's elements such that they appear to be on "top" of
 * other elements, and which capture all mouse events dispatched to the
 * screen (see: 'addModalDialogBox').
 *
 * This class will re-throw intercepted 'WawtException' exceptions after
 * extending the diagnostic message with the sceen's "name".  Note that
 * most exceptions result from erroneous layout directives.
 *
 * Finally, this class is NOT thread-safe, and has no exception guarantee.
 */
class Screen {
  public:
    // PUBLIC TYPES
    using SetTimerCb = std::function<void(std::chrono::milliseconds,
                                          std::function<void()>&&)>;
  private:
    // PRIVATE TYPES

    // PRIVATE CLASS MEMBERS

    // PRIVATE DATA MEMBERS
    bool                    d_modalActive   = false;
    SetTimerCb              d_setTimer{};

  protected:
    // PROTECTED TYPES
    using Widgets       = Widget::Children;

    // PROTECTED CONSTRUCTOR
    /**
     * @brief Initialize protected data members of the derived object.
     *
     * @param adapter Pointer to the object implementing the draw protocol.
     */
    Screen(DrawProtocol *adapter) : d_adapter(adapter), d_name(), d_screen() {}

    // PROTECTED MANIPULATOR

    //! Cancel any pending timed event.
    void cancelTimedEvent() {
        setTimedEvent(std::chrono::milliseconds{},
                      std::function<void()>());
    }

    /**
     * @brief Invoke a callback after approximate time interval has elapsed
     *
     * @param interval The elapsed time in milliseconds.
     * @param callback Function called after the time elapses.
     *
     * @return 'true' if the "timer" can be set, otherwise 'false'.
     *
     * Note that only one outstanding timed callback can be in effect at
     * any point in time.
     */
    bool setTimedEvent(std::chrono::milliseconds interval,
                       std::function<void()>     callback) {
        if (d_setTimer) {
            d_setTimer(interval, std::move(callback));
            return true;
        }
        return false;
    }

    // PROTECTED DATA MEMBERS
    DrawProtocol       *d_adapter = nullptr; ///< Draw protocol provider.
    std::string         d_name;              ///< Identifier for the screen.
    Widget              d_screen;            ///< Root WAWT element.

  public:

    // PUBLIC CONSTANTS

    //! Scale normalization adjustment constants.
    static constexpr Normalize kOUTER  = Layout::Normalize::eOUTER;
    static constexpr Normalize kMIDDLE = Layout::Normalize::eMIDDLE;
    static constexpr Normalize kINNER  = Layout::Normalize::eINNER;

    // PUBLIC MANIPULATORS

    /**
     * @brief Extend the screen definition with a pop dialog box.
     *
     * @param widgets A vector of "widgets" that are the dialog's contents.
     * @param width The width of the panel where 1.0 is the screen width.
     * @param height The height of the panel where 1.0 is the screen height.
     * @param borderThickness The panels border thickness, defaulting to 2.
     * @param options Panel display options defaulting to the screen options.
     *
     * @return The widget ID of the first widget in 'panel' on success.
     * 
     * The 'width' and 'height' will be used to create a centered panel with
     * those dimensions (see: 'Layout::centered'). Their values must
     * be in the range of 0.1 and 1.0; the returned widget ID is unset if
     * they are not. If a modal pop-up is already active, then this method
     * replaces that one.
     * Note that the screen is overlayed with a transparent widget which
     * prevents any user interface element other than the pop-up 'panel'
     * from receiving input events.
     */
    WidgetId addModalDialogBox(Widgets&&       widgets,
                               double          width           = 0.33,
                               double          height          = 0.33,
                               double          borderThickness = 2.0,
                               std::any        options         = std::any());

    /**
     * @brief Draw the current screen user interface elements.
     *
     * @throws WawtException Rethrows intercepted WawtExeption with the
     * screen name attached to the exception message.
     *
     * Draw the user interface elements contained by the screen (unless
     * marked as "hidden") in the order of declaration.  Thus, a widget's
     * drawn appearance can be obscured by a subsequent widget's.
     */
    void draw() {
        try {
            d_screen.draw(d_adapter);
        }
        catch (WawtException caught) {
            throw WawtException("Painting: '" + d_name + "', "
                                                            + caught.what());
        }
    }

    /**
     * @brief Discard the currently active "modal dialog box".
     *
     * CAUTION: This method is often called when a button (e.g. "Close") on
     * the dialog panel is pressed. However, the panel and all its contained
     * elements will have been discarded when this method returns. Therefore,
     * the routine calling this method must not subsequently make use of
     * context that was lost (i.e. the caller's lambda captures must be
     * copied to local variables if they are needed after this method
     * is called).
     */
    void dropModalDialogBox() {
        if (d_modalActive) {
            d_screen.popDialog();
            d_modalActive = false;
        }
    }

    /**
     * @brief When the mouse button is pressed down, call the selected widget.
     *
     * @param x The horizontal position relative to the screen's left edge.
     * @param y The veritical position relative to the screen's upper edge.
     * @return A callback to be called with the mouse button is released.
     *
     * @throws WawtException Rethrows intercepted WawtExeption with the
     * screen name attached to the exception message.
     *
     * Each widget is tested (unless marked as "disabled") in the reverse
     * order of declaration to see if it contains the point identified by
     * the 'x' and 'y' coordinates.  If so, no further widgets are tested,
     * and the selected widget's handler is called which returns a callback
     * that is invoked with the mouse button is released. If no widget passes
     * the selection test, then an "empty" callback is returned (and evaluates
     * to 'false' in a boolean context).
     */
    EventUpCb downEvent(int, int) {
        return EventUpCb(); // TBF
#if 0
        try {
            auto cb = d_screen.downEvent(x, y);
            if (!cb) {
                return cb;
            }
            refresh();
            return [this, cb](int xup, int yup, bool up) { // !!HACK - FIX
                auto ret = cb(xup, yup, up);
                refresh();
                return ret;
            };
        }
        catch (WawtException caught) {
            throw WawtException("Click on screen '" + d_name + "', "
                                                            + caught.what());
        }
#endif
    }

    /**
     * @brief Scale the screen so it conforms to the new width and height.
     *
     * @param newWidth  The new width value (optionally the current width).
     * @param newHeight The new height value (optionally the current height).
     *
     * This method should also be called if the mapping of 'TextId' values
     * to strings change (e.g. a language change). The 'activate' method
     * must be called before this method can be used.
     */
    void resize(double newWidth = 0, double newHeight = 0) {
        d_screen.resizeRoot(d_adapter, 
                            newWidth  ? newWidth  : width(),
                            newHeight ? newHeight : height());
    }
                
    /**
     * @brief Complete initialization of this object.
     *
     * @param name Name to add to intercepted exceptions before rethrowing.
     * @param setTimer Callback to establish/cancel a timed event.
     * @param adapter Pointer to the object implementing the draw protocol.
     *
     * This method should be called immediately after the screen is created.
     */
    void wawtScreenSetup(std::string_view   name,
                         const SetTimerCb&  setTimer,
                         DrawProtocol      *adapter = nullptr) {
        if (adapter) {
            d_adapter = adapter;
        }
        d_setTimer  = setTimer;
        d_name.assign(name.data(), name.length());
    }

    // PUBLIC ACCESSSOR

    //! Access the screen's height (requires 'activate' to have been performed).
    int height() const {
        return d_screen.drawData().d_rectangle.d_height;
    }

    //! Access the screen's "name" (requires 'setup' to have been performed).
    const std::string& name() const {
        return d_name;
    }

    //! Serialize the screen's definition to the output stream 'os'
    void serializeScreen(std::ostream& os) const {
        d_screen.serialize(os);
    }

    //! Access the screen's width (requires 'activate' to have been performed).
    double width() const {
        return d_screen.drawData().d_rectangle.d_width;
    }
};

                            //=================
                            // class ScreenImpl
                            //=================

/**
 * @brief Base class that is extended to implement a user interface screen.
 *
 * The 'ScreenImpl' class defines the framework a derived class should
 * adhear to in the implementation of a user interface screen.
 *
 * The derived class should inherit from this class specialized by the
 * derived class's name (this is the Curious Recursive Template Pattern),
 * and must implement the 'createScreenPanel' method.  It can, optionally,
 * also implement an 'initialize' method.  The first method defines the
 * widgets that make up the screen, and the second provides the application
 * an opportunity to access those widgets after the screen has been processed
 * by the framework.  Most commonly, the 'initialize' method is where pointers
 * to widgets (obtained using the 'lookup' method in this class) are saved in
 * member variables for later use.
 *
 * After the screen is defined and initialized (via this class's 'setup'
 * method), it can be displayed to the user using the 'activate' method.
 * This method requires the derived class to implement a 'resetWidgets'
 * method which is responsible for setting the state of the screen.
 * Typically, a screen will be redisplayed after other screens have
 * been shown, and the 'resetWidgets' method can return the screen
 * to an appropriate state (the state of the screen when
 * last displayed is usually not desired).
 *
 * Finally, this class is NOT thread-safe, and has no exception guarantee.
 */
template<class Derived, class Option>
class ScreenImpl : public Screen {
    template<class T>
    using initialize_t = decltype(std::declval<T>().initialize());

    template<class T>
    constexpr static bool has_initialize
                     = std::experimental::is_detected_v<initialize_t,T>;

    template<class T>
    using notify_t = decltype(std::declval<T>().notify());

    template<class T>
    constexpr static bool has_notify
                     = std::experimental::is_detected_v<notify_t,T>;

    // PRIVATE CLASS MEMBERS
    static Option optionCast(const std::any& option) {
        if (option.has_value()) {
            return std::any_cast<Option>(option);
        }
        return Option();
    }

    // PRIVATE MANIPULATORS

  public:
    // PUBLIC TYPES

    // PUBLIC CLASS MEMBERS

    // PUBLIC MANIPULATORS
    /**
     * @brief Activate a screen so its 'draw' and other methods can be called.
     *
     * @param width The screen width.
     * @param height The screen height.
     * @param args The parameters to be passed to the 'resetWidgets' method.
     */
    template<typename... Types>
    void activate(double width, double height, Types&... args);

    /**
     * @brief Initialize the screen's definition.
     *
     * @param args The parameters the application needs to define the screen.
     *
     * This method calls the application's 'createScreenPanel', and (if
     * defined) its 'initialize' methods.  This object is prepared to be
     * activated on return.
     */
    template<typename... Types>
    void setup(Types&... args);

  protected:
    // PROTECTED CONSTRUCTORS

    //! Initialize the base class with a close function object.
    ScreenImpl(DrawProtocol *adapter = nullptr);

    // PROTECTED MANIPULATORS
    /**
     * @brief Return a reference to a widget based on it's ID.
     *
     * @param id The widget's ID.
     * @return A reference to the corresponding 'Widget' (if any).
     *
     * @throws WawtException Throws WawtExeption if ID is not located.
     *
     * The widget ID which is assigned during 'setup' should not contain a
     * "relative" value unless the desired widget is contained directly
     * by the root panel.
     */
    Widget& lookup(WidgetId id);

    // PROTECTED ACCESSORS

    //! Get the draw options for the named widget class.
    Option defaultOptions(const std::string& name)   const noexcept {
        return optionCast(WawtEnv::instance()->defaultOptions(name));
    }

    // PROTECTED DATA MEMBERS
};

inline WidgetId
Screen::addModalDialogBox(Widgets&&       widgets,
                          double          width,
                          double          height,
                          double          borderThickness,
                          std::any        options)
{
    auto ret = WidgetId{};

    if (width <= 1.0 && height <= 1.0 && width >  0.1 && height >  0.1) {
        if (d_modalActive) {
            dropModalDialogBox();
        }
        d_modalActive = true;

        if (!options.has_value()) {
            options = WawtEnv::instance()->defaultOptions(WawtEnv::sScreen);
        }
        ret = d_screen.pushDialog(
                d_adapter,
                Widget(WawtEnv::sDialog,
                       Layout::centered(width, height).border(borderThickness))
                .options(std::move(options))
                .addChildren(std::move(widgets)));
    }
    return ret;                                                       // RETURN
}

template<class Derived,class Option>
inline
ScreenImpl<Derived,Option>::ScreenImpl(DrawProtocol *adapter)
: Screen(adapter)
{
}

template<class Derived,class Option>
template<typename... Types>
inline void
ScreenImpl<Derived,Option>::activate(double         width,
                                     double         height,
                                     Types&...      args)
{
    try {
        assert(d_adapter);
        resize(width, height);
        static_cast<Derived*>(this)->resetWidgets(args...);
    }
    catch (WawtException caught) {
            std::ostringstream os;
            os << "Activating screen ''" << d_name << "', "
               << caught.what() << '\n';
            serializeScreen(os);
        throw WawtException(os.str());                                 // THROW
    }
    return;                                                           // RETURN
}

template<class Derived,class Option>
inline Widget&
ScreenImpl<Derived,Option>::lookup(WidgetId id)
{
    auto widget_p = d_screen.lookup(id);

    if (widget_p == nullptr) {
        std::ostringstream os;
        os << "Screen '" << d_name << "' lookup of widget ID " << id.value()
           << " failed.\n";
        serializeScreen(os);
        throw WawtException(os.str());                                 // THROW
    }
    return *widget_p;                                                 // RETURN
}

template<class Derived,class Option>
template<typename... Types>
void
ScreenImpl<Derived,Option>::setup(Types&...  args)
{

    try {
        d_screen=reinterpret_cast<Derived*>(this)->createScreenPanel(args...);
        d_screen.assignWidgetIds();
    }
    catch (WawtException caught) {
        std::ostringstream os;
        os << "Setup of screen '" << d_name << "', " << caught.what() << '\n';
        serializeScreen(os);
        throw WawtException(os.str());                                 // THROW
    }
    
    if constexpr (has_initialize<Derived>) {
        try {
            reinterpret_cast<Derived*>(this)->initialize();
        }
        catch (WawtException caught) {
            std::ostringstream os;
            os << "Initializing screen ''" << d_name << "', "
               << caught.what() << '\n';
            serializeScreen(os);
            throw WawtException(os.str());                             // THROW
        }
    }
    return;                                                           // RETURN
}

} // end Wawt namespace

#endif
// vim: ts=4:sw=4:et:ai
