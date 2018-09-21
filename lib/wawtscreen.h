/** @file wawtscreen.h
 *  @brief Base class for forms in the WAWT framework.
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

#ifndef BDS_WAWTSCREEN_H
#define BDS_WAWTSCREEN_H

#include "wawt.h"

#include <any>
#include <chrono>
#include <experimental/type_traits>
#include <sstream>
#include <string>
#include <typeinfo>
#include <utility>

namespace BDS {

                            //=================
                            // class WawtScreen
                            //=================

/**
 * @brief Exposes operational aspects of a user interface screen.
 *
 * The 'WawtScreen' class provides an interface for managing user interface
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
 * This class will re-throw intercepted 'Wawt::Exception' exceptions after
 * extending the diagnostic message with the sceen's "name".  Note that
 * most exceptions result from erroneous layout directives.
 *
 * Finally, this class is NOT thread-safe, and has no exception guarantee.
 */
class WawtScreen {
  public:
    // PUBLIC TYPES
    using SetTimerCb = std::function<void(std::chrono::milliseconds,
                                          std::function<void()>&&)>;
  private:
    // PRIVATE TYPES

    // PRIVATE DATA MEMBERS
    bool                    d_modalActive = false;
    SetTimerCb              d_setTimer{};

  protected:
    // PROTECTED TYPES
    using ButtonBar     = Wawt::ButtonBar;
    using Button        = Wawt::Button;
    using Canvas        = Wawt::Canvas;
    using Label         = Wawt::Label;
    using List          = Wawt::List;
    using Panel         = Wawt::Panel;
    using TextEntry     = Wawt::TextEntry;

    using Layout        = Wawt::Layout;
    using InputHandler  = Wawt::InputHandler;
    using TextString    = Wawt::TextString;
    using DrawSettings  = Wawt::DrawSettings;

    using FocusCb       = Wawt::FocusCb;

    using ActionType    = Wawt::ActionType;
    using Align         = Wawt::Align;
    using Enablement    = Wawt::Enablement;
    using Normalize     = Wawt::Normalize;
    using Vertex        = Wawt::Vertex;
    using WidgetId      = Wawt::WidgetId;

    // PROTECTED CONSTRUCTOR
    /**
     * @brief Initialize protected data members of the derived object.
     */
    WawtScreen() : d_wawt(), d_name(), d_screen() { }

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
    Wawt                   *d_wawt;        ///< Holder of the WAWT adapters.
    std::string             d_name;        ///< Identifier for the screen.
    Wawt::Panel             d_screen;      ///< Root WAWT element.

  public:

    // PUBLIC CONSTANTS

    //! Border thickness placeholder that defaults to the assigned default.
    constexpr static const Wawt::OptInt kDEFAULT_BORDER{-1};

    //! Scale normalization adjustment constants.
    constexpr static const Wawt::Normalize kOUTER  = Wawt::Normalize::eOUTER;
    constexpr static const Wawt::Normalize kMIDDLE = Wawt::Normalize::eMIDDLE;
    constexpr static const Wawt::Normalize kINNER  = Wawt::Normalize::eINNER;

    // PUBLIC MANIPULATORS

    /**
     * @brief Extend the screen definition with an element which is drawn last.
     *
     * @param panel Contains user interface elements to "pop-up".
     * @param width The width of the panel where 1.0 is the screen width.
     * @param height The height of the panel where 1.0 is the screen height.
     * @param borderThickness The panels border thickness, defaulting to 2.
     * 
     * The 'width' and 'height' will be used to create a centered panel with
     * those dimensions (see: 'Wawt::Lahyout::centered'). Their values must
     * be in the range of 0.1 and 1.0.
     * Note that the screen is overlayed with a transparent canvas which
     * prevents any user interface element other than the pop-up 'panel'
     * from receiving events.
     */
    void addModalDialogBox(Wawt::Panel   panel,
                           double        width,
                           double        height,
                           int           borderThickness = 2);

    /**
     * @brief Draw the current screen user interface elements.
     *
     * @throws Wawt::Exception Rethrows intercepted Wawt::Exeption with the
     * screen name attached to the exception message.
     *
     * Draw the user interface elements contained by the screen (unless
     * marked as "hidden") in the order of declaration.  Thus, a widget's
     * drawn appearance can be obscured by a subsequent widget's.
     */
    void draw() {
        try {
            d_wawt->draw(d_screen);
        }
        catch (Wawt::Exception caught) {
            throw Wawt::Exception("Painting: '" + d_name + "', "
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
            Wawt::removePopUp(&d_screen);
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
     * @throws Wawt::Exception Rethrows intercepted Wawt::Exeption with the
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
    Wawt::EventUpCb downEvent(int x, int y) {
        try {
            auto cb = d_screen.downEvent(x, y);
            if (!cb) {
                return cb;
            }
            refresh();
            return [this, cb](int xup, int yup, bool up) {
                auto ret = cb(xup, yup, up);
                refresh();
                return ret;
            };
        }
        catch (Wawt::Exception caught) {
            throw Wawt::Exception("Click on screen '" + d_name + "', "
                                                            + caught.what());
        }
    }

    /**
     * @brief Refresh font assignments and text metrics.
     *
     * A change to a widget's (e.g. a label) associated text may require
     * that the character size used when the text is rendered be updated.
     * It may also require updating the dimensions of the text so that
     * the associated alignment is correctly implemented.
     * This method is usually called after handling mouse events
     * as this often triggers text changes.
     */
    void refresh() {
        d_wawt->refreshTextMetrics(&d_screen);
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
        d_wawt->resizeRootPanel(&d_screen,
                                newWidth  ? newWidth  : width(),
                                newHeight ? newHeight : height());
    }
                
    /**
     * @brief Complete initialization of this object.
     *
     * @param wawt Pointer to the shared toolkit instance.
     * @param name Name to add to intercepted exceptions before rethrowing.
     * @param setTimer Callback to establish/cancel a timed event.
     *
     * This method should be called immediately after the screen is created.
     */
    void wawtScreenSetup(Wawt              *wawt,
                         std::string_view   name,
                         const SetTimerCb&  setTimer) {
        d_wawt      = wawt;
        d_setTimer  = setTimer;
        d_name.assign(name.data(), name.length());
    }

    // PUBLIC ACCESSSOR

    //! Access the screen's height (requires 'activate' to have been performed).
    int height() const {
        return d_screen.adapterView().height();
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
        return d_screen.adapterView().width();
    }
};

                            //=====================
                            // class WawtScreenImpl
                            //=====================

/**
 * @brief Base class that is extended to implement a user interface screen.
 *
 * The 'WawtScreenImpl' class defines the framework a derived class should
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
class WawtScreenImpl : public WawtScreen {
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
    /**
     * @brief Construct a list and the two buttons used in scrolling.
     *
     * @param list The list definition.
     * @param buttonsOnLeft 'true' if the scroll buttons are left of the list.
     * @param lines The number of "lines" to be scrolled per button press.
     * @returns A 'Panel" which contains the list and buttons.
     *
     * The returned panel will use the layout values from 'list' whose values
     * are redefined.  It will contain the otherwise unchanged list, and the
     * two scroll buttons whose position is controlled by the 'buttonsOnLeft'
     * flag.  Note that before use, the 'Wawt::setScrollableListStartingRow'
     * method must be called (possibly in the 'resetWidgets' method) with a
     * pointer to the returned panel (and typically '0' for the staring row).
     * This will initialize the scroll button's "hidden" attribute
     * (which is automatically maintained subsequently).
     */
    static Panel ScrollableList(List      list,
                                bool      buttonsOnLeft = true,
                                unsigned  lines         = 1) {
        return Wawt::scrollableList(list, buttonsOnLeft, lines);
    }

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
    WawtScreenImpl();

    // PROTECTED MANIPULATORS
    /**
     * @brief Return a reference to a widget based on it's ID.
     *
     * @param id The widget's ID.
     * @return A reference to the selected 'WidgetType' object.
     *
     * @throws Wawt::Exception Rethrows intercepted Wawt::Exeption with the
     * screen name attached to the exception message.  This occurs when
     * no widget is selected by the ID, or the widget is not of type:
     * 'WidgetType'.
     *
     * The widget ID which is assigned during 'setup' should not contain a
     * "relative" value unless the desired widget is contained directly
     * by the root panel.
     */
    template<typename WidgetType>
    WidgetType& lookup(WidgetId id);

    // PROTECTED ACCESSORS
    //! Get the draw options for the root screen panel.
    Option defaultScreenOptions()    const {
        return optionCast(d_wawt->defaultScreenOptions());
    }

    //! Get the draw options for the 'Canvas' widget.
    Option defaultCanvasOptions()    const {
        return optionCast(d_wawt->defaultCanvasOptions());
    }

    //! Get the draw options for the 'TextEntry' widget.
    Option defaultTextEntryOptions() const {
        return optionCast(d_wawt->defaultTextEntryOptions());
    }

    //! Get the draw options for the 'Label' widget.
    Option defaultLabelOptions()     const {
        return optionCast(d_wawt->defaultLabelOptions());
    }

    //! Get the draw options for the 'Button' widget not owned by a 'List'.
    Option defaultButtonOptions()    const {
        return optionCast(d_wawt->defaultButtonOptions());
    }

    //! Get the draw options for the 'ButtonBar' widget.
    Option defaultButtonBarOptions() const {
        return optionCast(d_wawt->defaultButtonBarOptions());
    }

    //! Get the draw options for the 'List' widget of list type 'type'.
    Option defaultListOptions(Wawt::ListType type)      const {
        return optionCast(d_wawt->defaultListOptions(type));
    }

    //! Get the draw options for the 'Panel' widget.
    Option defaultPanelOptions()     const {
        return optionCast(d_wawt->defaultPanelOptions());
    }

    // PROTECTED DATA MEMBERS
};

inline void
WawtScreen::addModalDialogBox(Wawt::Panel   panel,
                              double        width,
                              double        height,
                              int           borderThickness)
{
    if (!d_modalActive && width <= 1.0 && height <= 1.0
                       && width >  0.1 && height >  0.1) {
        if (!panel.drawView().options().has_value()) {
            panel.drawView().options() = d_wawt->getWidgetOptionDefaults()
                                               .d_screenOptions;
        }
        panel.layoutView()
            = Wawt::Layout::centered(width, height).border(borderThickness);
        d_wawt->popUpModalDialogBox(&d_screen, std::move(panel));
        d_modalActive = true;
    }
}

template<class Derived,class Option>
inline
WawtScreenImpl<Derived,Option>::WawtScreenImpl() : WawtScreen()
{
}

template<class Derived,class Option>
template<typename... Types>
inline void
WawtScreenImpl<Derived,Option>::activate(double         width,
                                         double         height,
                                         Types&...      args)
{
    try {
        static_cast<Derived*>(this)->resetWidgets(args...);
        d_wawt->resizeRootPanel(&d_screen, width, height);
    }
    catch (Wawt::Exception caught) {
            std::ostringstream os;
            os << "Activating screen ''" << d_name << "', "
               << caught.what() << '\n';
            serializeScreen(os);
        throw Wawt::Exception(os.str());                              // THROW
    }
    return;                                                           // RETURN
}

template<class Derived,class Option>
template<typename WidgetType>
inline WidgetType&
WawtScreenImpl<Derived,Option>::lookup(WidgetId id)
{
    return d_screen.lookup<WidgetType>(id,
                                       d_name
                                       + ": "
                                       + typeid(WidgetType).name());  // RETURN
}

template<class Derived,class Option>
template<typename... Types>
void
WawtScreenImpl<Derived,Option>::setup(Types&...  args)
{

    try {
        d_screen=reinterpret_cast<Derived*>(this)->createScreenPanel(args...);
        d_wawt->resolveWidgetIds(&d_screen);
    }
    catch (Wawt::Exception caught) {
        std::ostringstream os;
        os << "Setup of screen '" << d_name << "', " << caught.what() << '\n';
        serializeScreen(os);
        throw Wawt::Exception(os.str());                               // THROW
    }
    
    if constexpr (has_initialize<Derived>) {
        try {
            reinterpret_cast<Derived*>(this)->initialize();
        }
        catch (Wawt::Exception caught) {
            std::ostringstream os;
            os << "Initializing screen ''" << d_name << "', "
               << caught.what() << '\n';
            serializeScreen(os);
            throw Wawt::Exception(os.str());                          // THROW
        }
    }
    return;                                                           // RETURN
}

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
