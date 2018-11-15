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

#include "wawt/wawtenv.h"
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
     */
    Screen() : d_name(), d_screen(WawtEnv::sScreen,{}) {}

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
    std::string         d_name;              ///< Identifier for the screen.
    Widget              d_screen;            ///< Root WAWT element.

  public:

    // PUBLIC CONSTANTS

    // PUBLIC MANIPULATORS

    /**
     * @brief Extend the screen definition with a pop dialog box.
     *
     * @param dialog  A panel containing the dialog to be displayed.
     *
     * @returns The widget ID assigned to the dialog panel on success.
     * 
     * Note that the screen is overlayed with a transparent widget which
     * prevents any user interface element other than the pop-up 'panel'
     * from receiving input events.
     * If the 'dialogBox' is not identified with the correct "option name"
     * ('WawtEnv::sDialog'), the widget ID returned is not set.
     */
    WidgetId addModalDialogBox(Widget&& dialogBox);

    /**
     * @brief Clear any exising widget "focus" on screen.
     */
    void clearFocus() {
        d_screen.focus();
    }

    /**
     * @brief Draw the current screen user interface elements.
     *
     * @param adapter Optional adapter to use when drawing.
     *
     * @throws WawtException Rethrows intercepted WawtExeption with the
     * screen name attached to the exception message.
     *
     * Draw the user interface elements contained by the screen (unless
     * marked as "hidden") in the order of declaration.  Thus, a widget's
     * drawn appearance can be obscured by a subsequent widget's.
     */
    void draw(DrawProtocol *adapter = WawtEnv::drawAdapter()) {
        try {
            d_screen.draw(adapter);
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
    EventUpCb downEvent(double x, double y) {
        try {
            return d_screen.downEvent(x, y);
        }
        catch (WawtException caught) {
            throw WawtException("Click on screen '" + d_name + "', "
                                                            + caught.what());
        }
    }

    /**
     * @brief Route the input character to the widget with "focus".
     *
     * @param input The input character.
     * @return Return 'true' if input was processed, 'false' otherwise.
     */
    bool inputEvent(Char_t input) {
        if (d_screen.inputMethod()) {
            d_screen.inputEvent(input);
            return true;
        }
        return false;
    }

    /**
     * @brief Scale the screen so it conforms to the new width and height.
     *
     * @param newWidth  The new width value (optionally the current width).
     * @param newHeight The new height value (optionally the current height).
     * @param adapter Optional adapter to use when "laying" out the screen.
     *
     * The 'activate' method must be called before this method can be used.
     */
    void resize(double        newWidth  = 0,
                double        newHeight = 0,
                DrawProtocol *adapter   = WawtEnv::drawAdapter()) {
        d_screen.resizeScreen(newWidth  ? newWidth  : width(),
                              newHeight ? newHeight : height(),
                              adapter);
    }
                
    /**
     * @brief Complete initialization of this object.
     *
     * @param name Name to add to intercepted exceptions before rethrowing.
     * @param setTimer Callback to establish/cancel a timed event.
     *
     * This method should be called immediately after the screen is created.
     */
    void screenSetup(std::string_view name, const SetTimerCb& setTimer) {
        d_setTimer  = setTimer;
        d_name.assign(name.data(), name.length());
    }
                
    /**
     * @brief Update text strings with current layout results.
     *
     * @param widget Optional widget to synchronize. Default is all widgets.
     *
     * This method is called as a part of 'activate', but should also be
     * called if there was a change that would modify the values returned
     * by any text layout string functor. A call to 'resize()' may have to
     * follow if the text changes the width of the displayed text.
     */
    void synchronizeTextView(Widget *widget = nullptr) {
        if (widget) {
            widget->synchronizeTextView(false);
        }
        else {
            d_screen.synchronizeTextView(true);
        }
    }

    // PUBLIC ACCESSSOR

    //! Access the screen's height (requires 'activate' to have been performed).
    int height() const {
        return d_screen.layoutData().d_bounds.d_height;
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
        return d_screen.layoutData().d_bounds.d_width;
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

    // PUBLIC CONSTRUCTORS

    // To insure that 'this' can be used in lambda captures often found in
    // widget callbacks, delete the move and copy constructors and assignments.

    ScreenImpl(ScreenImpl&&)                        = delete;
    ScreenImpl(const ScreenImpl&)                   = delete;

    // PUBLIC MANIPULATORS

    ScreenImpl& operator=(ScreenImpl&&)             = delete;
    ScreenImpl& operator=(const ScreenImpl&)        = delete;

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
    ScreenImpl();

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
        return optionCast(WawtEnv::defaultOptions(name));
    }

    // PROTECTED DATA MEMBERS
};

inline 
WidgetId
Screen::addModalDialogBox(Widget&& dialogBox)
{
    if (d_modalActive) {
        dropModalDialogBox();
    }
    auto widgetId = d_screen.pushDialog(std::move(dialogBox));
    d_modalActive = widgetId.isSet();
    return widgetId;                                                  // RETURN
}

template<class Derived,class Option>
inline
ScreenImpl<Derived,Option>::ScreenImpl()
: Screen()
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
        static_cast<Derived*>(this)->resetWidgets(args...);
        d_screen.synchronizeTextView();
        resize(width, height);
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
        auto app = reinterpret_cast<Derived*>(this);
        d_screen= app->createScreenPanel(args...).optionName(WawtEnv::sScreen);
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
