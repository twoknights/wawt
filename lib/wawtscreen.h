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
#include <experimental/type_traits>
#include <string>
#include <typeinfo>
#include <utility>

namespace BDS {

                            //==================
                            // struct WawtScreen
                            //==================

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
  protected:
    // PROTECTED TYPES
    using ButtonBar    = Wawt::ButtonBar;
    using Button       = Wawt::Button;
    using Canvas       = Wawt::Canvas;
    using Label        = Wawt::Label;
    using List         = Wawt::List;
    using Panel        = Wawt::Panel;
    using TextEntry    = Wawt::TextEntry;

    using Layout       = Wawt::Layout;
    using InputHandler = Wawt::InputHandler;
    using TextString   = Wawt::TextString;
    using DrawSettings = Wawt::DrawSettings;

    using Align        = Wawt::Align;
    using Enablement   = Wawt::Enablement;
    using Metric       = Wawt::Metric;
    using TieScale     = Wawt::TieScale;
    using Vertex       = Wawt::Vertex;
    using WidgetId     = Wawt::WidgetId;

    using CloseFn      = std::function<void(const std::function<void()>&)>;

    // PROTECTED CONSTRUCTOR
    /**
     * @brief Initialize protected data members of the derived object.
     *
     * @param closeFn Call into "impl" 'closeWindow' with bound callback.
     */
    WawtScreen(CloseFn&& closeFn)
        : d_wawt()
        , d_name()
        , d_screen()
        , d_close(std::move(closeFn)) { }

    // PROTECTED MANIPULATOR

    // PROTECTED DATA MEMBERS
    Wawt                  *d_wawt;           ///< Holder of the WAWT adapters.
    std::string            d_name;           ///< Identifier for the screen.
    Wawt::Panel            d_screen;         ///< Root WAWT element.
    CloseFn                d_close;          ///< Call into Impl close method.

  public:
    // PUBLIC CONSTANTS
    /**
     * @brief Screen layout constants.
     *
     * The layout of user interface elements is based on an origin lieing
     * at the center of a rectangular region identified by a widget ID.
     * The distance from the center to the side of the recangle is defined
     * as having a length of one with "left/up" being negative, and
     * "right/down" is positive (each direction is independently scaled).
     *
     * The following constants provide a means of specifying locations that
     * can be more intuitive by naming the "left/up" edge as the "begining",
     * followed by 25%, 33%, mid-point, 66%, 75%, and "end-point" (being the
     * opposite edge from the beginning).
     */
    constexpr static const Metric kLINE_BEG   = Metric(-1.0);
    constexpr static const Metric kLINE_25    = Metric(-0.5);
    constexpr static const Metric kLINE_33    = Metric(-1.0/3.0);
    constexpr static const Metric kLINE_MID   = Metric( 0.0);
    constexpr static const Metric kLINE_66    = Metric( 1.0/3.0);
    constexpr static const Metric kLINE_75    = Metric( 0.5);
    constexpr static const Metric kLINE_END   = Metric( 1.0);

    /**
     * @brief Additional screen layout constants.
     *
     * The following constants define vertices on the rectangular area
     * of a widget.
     */
    constexpr static const Vertex kUPPER_LEFT    {kLINE_BEG, kLINE_BEG};
    constexpr static const Vertex kUPPER_CENTER  {kLINE_MID, kLINE_BEG};
    constexpr static const Vertex kUPPER_RIGHT   {kLINE_END, kLINE_BEG};
    constexpr static const Vertex kCENTER_LEFT   {kLINE_BEG, kLINE_MID};
    constexpr static const Vertex kCENTER_CENTER {kLINE_MID, kLINE_MID};
    constexpr static const Vertex kCENTER_RIGHT  {kLINE_END, kLINE_MID};
    constexpr static const Vertex kLOWER_LEFT    {kLINE_BEG, kLINE_END};
    constexpr static const Vertex kLOWER_CENTER  {kLINE_MID, kLINE_END};
    constexpr static const Vertex kLOWER_RIGHT   {kLINE_END, kLINE_END};

    //! Relative widget ID that references the containing panel.
    constexpr static const WidgetId&   kPARENT          = Wawt::kPARENT;

    //! Border thickness placeholder that defaults to the assigned default.
    constexpr static const Wawt::OptInt kDEFAULT_BORDER{-1};

    //! Vertex edge adjustment constants.
    constexpr static const Wawt::ScaleBias kOUTER        = Wawt::eOUTER;
    constexpr static const Wawt::ScaleBias kINNER        = Wawt::eINNER;
    constexpr static const Wawt::ScaleBias kOUTER1       = Wawt::eOUTER1;
    constexpr static const Wawt::ScaleBias kINNER1       = Wawt::eINNER1;

    // PUBLIC MANIPULATORS

    /**
     * @brief Extend the screen definition with an element which is drawn last.
     * @param panel Contains user interface elements to "pop-up".
     * 
     * Note that panel should only reference the "parent" widget which is the
     * screen in its layout directive.  Note that the screen is overlayed with
     * a transparent widget which prevents any user interface element other
     * than 'panel' from receiving any mouse event.
     */
    void addModalDialogBox(Wawt::Panel panel) {
        if (!panel.drawView().options().has_value()) {
            panel.drawView().options() = d_wawt->getWidgetOptionDefaults()
                                               .d_screenOptions;
        }
        d_wawt->popUpModalDialogBox(&d_screen, std::move(panel));
    }

    /**
     * @brief Pass on the window close request to the application.
     *
     * @param completeClose Callback invoked to close the native window.
     *
     * This method calls the screen's 'shutdown' method if defined, otherwise
     * it calls 'completeClose' and returns.  The shutdown method will be
     * passed the 'completeClose' callback which it must call when it is
     * ready to complete the close of the window. Note that if 'shutdown'
     * creates a modal dialog box requiring user input, the 'shutdown' method
     * should return without calling 'completeClose'.  But when user
     * interaction is finished, the method should be callsed (the dialog box
     * might hold a copy of the callback in the input event handler for this
     * purpose).
     */
    void close(const std::function<void()>& completeClose) {
        d_close(completeClose);
    }

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
        Wawt::removePopUp(&d_screen);
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
            refresh();
            return cb;
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
     * to strings change (e.g. a language change).
     */
    void resize(int newWidth = 0, int newHeight = 0) {
        d_wawt->resizeRootPanel(&d_screen,
                                double(newWidth  ? newWidth  : width()),
                                double(newHeight ? newHeight : height()));
    }

    /**
     * @brief Complete initialization of this object.
     *
     * @param wawt Pointer to the shared toolkit instance.
     * @param name Name to add to intercepted exceptions before rethrowing.
     *
     * This method should be called immediately after the screen is created.
     */
    void wawtScreenSetup(Wawt *wawt, std::string_view name) {
        d_wawt = wawt;
        d_name.assign(name.data(), name.length());
    }

    // PUBLIC ACCESSSOR
    //! Access the screen's height (requires 'setup' to have been performed).
    int height() const {
        return int(std::round(d_screen.adapterView().height()));
    }

    const std::string& name() const {
        return d_name;
    }

    //! Access the screen's width (requires 'setup' to have been performed).
    int width() const {
        return int(std::round(d_screen.adapterView().width()));
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
 * The 'createScreenPanel' method must specify the size of the root panel
 * that was used when the screen layout. This can be accomplished by using
 * the root panel's layout returned by calling the 'screenLayout' method.
 * These values are used to determine how the other offset values found in the
 * screen definition must be scaled for the current screen dimensions.
 * The application methods may, once 'setup' completes, reference the current
 * screen dimensions in the corresponding data members if neccessary.
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
    using shutdown_t = decltype(std::declval<T>().shutdown());

    template<class T>
    constexpr static bool has_shutdown
                     = std::experimental::is_detected_v<shutdown_t,T>;

    // PRIVATE CLASS MEMBERS
    static Option optionCast(const std::any& option) {
        if (option.has_value()) {
            return std::any_cast<Option>(option);
        }
        return Option();
    }

    // PRIVATE MANIPULATORS
    void closeWindow(const std::function<void()>& completeClose) {
        if constexpr (has_shutdown<Derived>) {
            reinterpret_cast<Derived*>(this)->shutdown(completeClose);
        }
        else {
            completeClose();
        }
    }

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
     * @param current The screen currently "drawn" and handling "events".
     * @param args The parameters to be passed to the 'resetWidgets' method.
     *
     * Note that for the first screen drawn by the application, the 'current'
     * screen should be the 'nullptr'.  The application screen being activated
     * will be resized to the dimensions of the 'current' screen before this
     * method returns.
     */
    template<typename... Types>
    void activate(const WawtScreen *current, Types&... args);

    /**
     * @brief Create the screen's definition.
     *
     * @param initialWidth  The current screen width.
     * @param initialHeight The current screen height.
     * @param args The parameters the application needs to define the screen.
     *
     * Calls the application's 'createScreenPanel', and (if defined) its
     * 'initialize' methods.  This object is prepared to be activated on
     * return.
     */
    template<typename... Types>
    void setup(int initialWidth, int initialHeight, Types&... args);

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
    /**
     * @brief Return the "box" layout for the screen's root panel.
     *
     * @param width The screen's width when layout was defined.
     * @param height The screen's height when layout was defined.
     * @return A 'Wawt::Layout' structure for use in the screen's root panel.
     *
     */
    Layout screenLayout(int width, int height) {
        return Layout({}, {kUPPER_LEFT, width-1, height-1});
    }

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

template<class Derived,class Option>
inline
WawtScreenImpl<Derived,Option>::WawtScreenImpl()
: WawtScreen([this](auto completeClose) { closeWindow(completeClose); })
{
}

template<class Derived,class Option>
template<typename... Types>
inline void
WawtScreenImpl<Derived,Option>::activate(const WawtScreen *current,
                                        Types&...        args)
{
    try {
        reinterpret_cast<Derived*>(this)->resetWidgets(args...);

        if (current) {
            d_wawt->resizeRootPanel(&d_screen,
                                    double(current->width()),
                                    double(current->height()));
        }
    }
    catch (Wawt::Exception caught) {
        throw Wawt::Exception("Activate screen '" + d_name
                                                 + "', "
                                                 + caught.what());     // THROW
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
WawtScreenImpl<Derived,Option>::setup(int        initialWidth,
                                      int        initialHeight,
                                      Types&...  args)
{
    d_screen = reinterpret_cast<Derived*>(this)->createScreenPanel(args...);

    try {
        d_wawt->resolveWidgetIds(&d_screen);
        d_wawt->resizeRootPanel(&d_screen,
                                double(initialWidth),
                                double(initialHeight));
    }
    catch (Wawt::Exception caught) {
        throw Wawt::Exception("Setup screen '" + d_name + "', "
                                                        + caught.what());
    }
    
    if constexpr (has_initialize<Derived>) {
        try {
            reinterpret_cast<Derived*>(this)->initialize();
        }
        catch (Wawt::Exception caught) {
            throw Wawt::Exception("Initialize screen '" + d_name + "', "
                                                       + caught.what());//THROW
        }
    }
    return;                                                           // RETURN
}

} // end BDS namespace

#endif
// vim: ts=4:sw=4:et:ai
