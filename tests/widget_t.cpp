// widget_t.cpp

#include <gtest/gtest.h>
#include <wawt/wawtenv.h>
#include <wawt/widget.h>

#include <cassert>
#include <sstream>
#include <iostream>

using namespace std::literals;
using namespace Wawt;

#ifdef WAWT_WIDECHAR
#define S(str) String_t(L"" str)  // wide char strings (std::wstring)
#define C(c) (L ## c)
#else
#undef  S
#undef  C
#define S(str) String_t(u8"" str)      // UTF8 strings  (std::string)
#define C(c) (U ## c)
#endif

TEST(Widget, Constructors)
{
    auto w1 = Widget("foo", {});
    auto& layout1 = w1.layoutData().d_layout;
    EXPECT_EQ(-1.0, layout1.d_thickness);
    EXPECT_EQ(Layout::Vertex::eNONE, layout1.d_pin);
    EXPECT_EQ(-1.0, layout1.d_upperLeft.d_sX);
    EXPECT_EQ(-1.0, layout1.d_upperLeft.d_sY);
    EXPECT_EQ(-1.0, layout1.d_lowerRight.d_sX);
    EXPECT_EQ(-1.0, layout1.d_lowerRight.d_sY);
    EXPECT_EQ(WidgetId::kPARENT,
              layout1.d_upperLeft.d_widgetRef.getWidgetId());
    EXPECT_EQ(WidgetId::kPARENT,
              layout1.d_lowerRight.d_widgetRef.getWidgetId());
    EXPECT_EQ(Layout::Normalize::eDEFAULT, layout1.d_upperLeft.d_normalizeX);
    EXPECT_EQ(Layout::Normalize::eDEFAULT, layout1.d_upperLeft.d_normalizeY);
    EXPECT_EQ(Layout::Normalize::eDEFAULT, layout1.d_lowerRight.d_normalizeX);
    EXPECT_EQ(Layout::Normalize::eDEFAULT, layout1.d_lowerRight.d_normalizeY);

    Widget *p = nullptr;
    auto ul = Layout::Position(1.0,
                               2.0,
                               1_w,
                               Layout::Normalize::eOUTER,
                               Layout::Normalize::eINNER);
    auto lr = Layout::Position(3.0,
                               4.0,
                               2_w,
                               Layout::Normalize::eMIDDLE,
                               Layout::Normalize::eOUTER);
    auto w2 = Widget("bar", &p, {ul, lr, Layout::Vertex::eCENTER_CENTER, 1.0});
    auto& layout2 = w2.layoutData().d_layout;
    EXPECT_EQ(&w2, p);
    EXPECT_EQ( 1.0, layout2.d_thickness);
    EXPECT_EQ(Layout::Vertex::eCENTER_CENTER, layout2.d_pin);
    EXPECT_EQ( 1.0, layout2.d_upperLeft.d_sX);
    EXPECT_EQ( 2.0, layout2.d_upperLeft.d_sY);
    EXPECT_EQ( 3.0, layout2.d_lowerRight.d_sX);
    EXPECT_EQ( 4.0, layout2.d_lowerRight.d_sY);
    EXPECT_TRUE(1_w == layout2.d_upperLeft.d_widgetRef.getWidgetId());
    EXPECT_TRUE(2_w == layout2.d_lowerRight.d_widgetRef.getWidgetId());
    EXPECT_EQ(Layout::Normalize::eOUTER, layout2.d_upperLeft.d_normalizeX);
    EXPECT_EQ(Layout::Normalize::eINNER, layout2.d_upperLeft.d_normalizeY);
    EXPECT_EQ(Layout::Normalize::eMIDDLE, layout2.d_lowerRight.d_normalizeX);
    EXPECT_EQ(Layout::Normalize::eOUTER, layout2.d_lowerRight.d_normalizeY);

    auto w3 = std::move(w2);
    EXPECT_EQ(&w3, p);

    std::string        closeTag;
    std::ostringstream os;
    Widget::defaultSerialize(os, &closeTag, w3, 0);
    os << closeTag;
    auto serialized =
"<bar id='0' rid='0'>\n"
"  <layout border='1' pin='4'>\n"
"    <ul sx='1' sy='2' widget='1_w' norm_x='0' norm_y='2'/>\n"
"    <lr sx='3' sy='4' widget='2_w' norm_x='1' norm_y='0'/>\n"
"  </layout>\n"
"  <text/>\n"
"  <installedMethods/>\n"
"</bar>\n"s;
    EXPECT_EQ(serialized, os.str());
}

TEST(Widget, AddChild)
{
    auto count = 0;

    auto w1 = Widget("child", {{-1,-1}, { 0, 0}, 1}); // 4 quadrants:
    auto w2 = Widget("child", {{ 0,-1}, { 1, 0}, 2}); //    ul, ur, lr, ll
    auto w3 = Widget("child", {{ 0, 0}, { 1, 1}, 3});
    auto w4 = Widget("child", {{-1, 0}, { 0, 1}, 4});

    auto w5 = Widget("root", {})
                .addMethod([&count](Widget*,Widget*) {++count;});

    w5.addChild(w1.clone());
    w5.addChild(w2.clone());

    EXPECT_EQ(2, count);

    auto screen= std::move(w5).addChild(std::move(w3)).addChild(std::move(w4));
    EXPECT_EQ(4, count);

    WawtEnv env;
    std::ostringstream os;
    auto adapter = Draw(os);

    EXPECT_EQ(nullptr, screen.screen());
    screen.assignWidgetIds();
    EXPECT_EQ(&screen, screen.screen());

    screen.resizeScreen(1280, 720, &adapter);
    screen.draw(&adapter);
    auto drawn =
"<root id='5' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1280' height='720' border='0'/>\n"
"  </draw>\n"
"</root>\n"
"<child id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='640' height='360' border='1'/>\n"
"  </draw>\n"
"</child>\n"
"<child id='2' rid=1'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='640' y='0' width='640' height='360' border='2'/>\n"
"  </draw>\n"
"</child>\n"
"<child id='3' rid=2'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='640' y='360' width='640' height='360' border='3'/>\n"
"  </draw>\n"
"</child>\n"
"<child id='4' rid=3'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='360' width='640' height='360' border='4'/>\n"
"  </draw>\n"
"</child>\n"s;
    EXPECT_EQ(drawn, os.str());
}

TEST(Widget, Text)
{
    using Tuple = WawtEnv::OptionTuple<int>;
    auto defaults = std::array{
        Tuple{ WawtEnv::sScreen, 0.0, 0 },
        Tuple{ WawtEnv::sLabel,  3.0, 0 }
    };
    WawtEnv obj(defaults);

    auto screen
        = Widget(WawtEnv::sScreen, {})  // screen layout is never used
          .addChild(Widget(WawtEnv::sLabel, Layout::centered(0.25, 0.25))
                    .text(S("'X' marks the spot:"), 1, TextAlign::eRIGHT)
                    .textMark(DrawData::BulletMark::eSQUARE, false)
                    .labelSelect(true));
    std::ostringstream os;
    auto adapter = Draw(os);
    screen.assignWidgetIds();
    screen.resizeScreen(1280, 720, &adapter);
    screen.draw(&adapter);
    auto drawn =
"<screen id='2' rid=0'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1280' height='720' border='0'/>\n"
"  </draw>\n"
"</screen>\n"
"<label id='1' rid=0'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='480' y='270' width='320' height='180' border='3'/>\n"
"    <text x='484' y='274' width='312' height='172' charSize='15' mark='1' left='false'/>\n"
"    <string>&apos;X&apos; marks the spot:</string>\n"
"  </draw>\n"
"</label>\n"s;
    EXPECT_EQ(drawn, os.str());
}

TEST(Widget, Dialog)
{
    using Tuple = WawtEnv::OptionTuple<int>;
    auto defaults = std::array{
        Tuple{ WawtEnv::sScreen, 0.0, 0 },
        Tuple{ WawtEnv::sLabel,  3.0, 0 }
    };
    WawtEnv obj(defaults);
    std::ostringstream os;
    auto adapter = Draw(os);

    auto screen
        = Widget(WawtEnv::sScreen, {});
    auto id = screen.pushDialog(Widget(WawtEnv::sDialog, {}), &adapter);
    EXPECT_FALSE(id.isSet());

    screen.assignWidgetIds();
    screen.resizeScreen(1280, 720, &adapter);
    EXPECT_EQ(0, screen.children().size());
    screen.popDialog();
    EXPECT_EQ(0, screen.children().size());

    id = screen.pushDialog(Widget(WawtEnv::sLabel, {}), &adapter);
    EXPECT_FALSE(id.isSet());
    EXPECT_EQ(0, screen.children().size());

    EXPECT_EQ(1, screen.widgetIdValue());
    id = screen.pushDialog(Widget(WawtEnv::sDialog, {}), &adapter);
    EXPECT_TRUE(id.isSet());
    EXPECT_FALSE(id.isRelative());
    EXPECT_EQ(1, id.value());
    EXPECT_EQ(2, screen.widgetIdValue());
    EXPECT_EQ(1, screen.children().size());

    screen.popDialog();
    EXPECT_EQ(0, screen.children().size());
    EXPECT_EQ(1, screen.widgetIdValue());

    id = screen.pushDialog(Widget(WawtEnv::sDialog,
                                  Layout::centered(0.25, 0.25))
                           .text(S("<POP!>"), 1, TextAlign::eRIGHT), &adapter);
    EXPECT_TRUE(id.isSet());
    EXPECT_EQ(1, screen.children().size());
    screen.draw(&adapter);
    auto drawn =
"<screen id='2' rid=0'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1280' height='720' border='0'/>\n"
"  </draw>\n"
"</screen>\n"
"<dialog id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='480' y='270' width='320' height='180' border='0'/>\n"
"    <text x='481' y='271' width='318' height='178' charSize='53'/>\n"
"    <string>&lt;POP!&gt;</string>\n"
"  </draw>\n"
"</dialog>\n"s;
    EXPECT_EQ(drawn, os.str());

    screen.popDialog();
    EXPECT_EQ(0, screen.children().size());
}

TEST(Widget, Methods)
{
    WawtEnv env();
    bool layout    = false;
    bool down      = false;
    bool draw      = false;
    bool serialize = false;
    bool child     = false;
    auto screen
        = Widget(WawtEnv::sScreen, {})  // screen layout is never used
          .addMethod( [&draw](Widget *, DrawProtocol *) { })
          .addMethod( [&serialize](std::ostream&, std::string*,
                                   const Widget&, unsigned int) { })
          .addChild(Widget("foo", {{-1.0,-1.0},{1.0,1.0}})
            .addMethod( [&layout](DrawData*, bool, const Widget&,
                                  const Widget&, const Widget::LayoutData&,
                                  DrawProtocol*) {
                            layout = true;
                        })
            .addMethod( [&down](float,float,Widget*) {
                            down = true;
                            return EventUpCb();
                        })
            .addMethod( [&draw](Widget *, DrawProtocol *) {
                            draw = true;
                        })
            .addMethod( [&serialize](std::ostream&, std::string*,
                                     const Widget&, unsigned int) {
                            serialize = true;
                        })
            .addMethod( [&child](Widget*,Widget*) {
                            child = true;
                        }));

    auto& w = screen.children().back();
    EXPECT_TRUE(bool(w.getInstalled<Widget::DownEventMethod>()));
    EXPECT_TRUE(bool(w.getInstalled<Widget::DrawMethod>()));
    EXPECT_TRUE(bool(w.getInstalled<Widget::LayoutMethod>()));
    EXPECT_TRUE(bool(w.getInstalled<Widget::NewChildMethod>()));
    EXPECT_TRUE(bool(w.getInstalled<Widget::SerializeMethod>()));
    
    std::ostringstream os1, os2;
    auto adapter = Draw(os1);

    screen.assignWidgetIds();
    screen.resizeScreen(1280, 720, &adapter);
    screen.downEvent(600,600);
    screen.draw(&adapter);
    screen.serialize(os2);
    screen.children().back().addChild(Widget("bar", {}));
    EXPECT_FALSE(down); // layout was not performed.
    EXPECT_TRUE(draw);
    EXPECT_TRUE(layout);
    EXPECT_FALSE(child); // can't add after assignment
    EXPECT_TRUE(serialize);

    EXPECT_TRUE(os1.str().empty()) << os1.str();
    EXPECT_TRUE(os2.str().empty()) << os2.str();

    w.setMethod(Widget::LayoutMethod());
    screen.resizeScreen(1280, 720, &adapter);
    screen.downEvent(600,600);
    EXPECT_TRUE(down);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "fast";
    return RUN_ALL_TESTS();
}

// vim: ts=4:sw=4:et:ai
