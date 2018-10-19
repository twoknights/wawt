// widgetfactory_t.cpp

#include <gtest/gtest.h>
#include <wawt/wawtenv.h>
#include <wawt/widgetfactory.h>

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

TEST(Factory, PushButton)
{
    auto os1        = std::ostringstream{};
    auto adapter    = Draw(os1);
    auto env        = WawtEnv(&adapter);
    Widget *pp, *p1, *p2;

    auto screen
        = panel(&pp, {})
            .addChild(pushButton(&p1, {{-1.0, -1.0},{-0.9,-0.9}},
                                 [](auto) {return FocusCb();},
                                 S("b1"), TextAlign::eLEFT, 1_F))
            .addChild(pushButton(&p2, {{-0.9, -0.9},{-0.8,-0.8}},
                                 [](auto) {return FocusCb();}, S("b2"), 1_F))
            .addChild(pushButton({{-0.8, -0.8},{-0.7,-0.7}},
                                 [](auto) {return FocusCb();}, S("b3"), 1_F));
    screen.assignWidgetIds();
    screen.resizeScreen(1000, 1000);

    auto os2 = std::ostringstream{};
    screen.serialize(os2);
    auto serialized =
"<panel id='4' rid='0'>\n"
"  <layout border='0'>\n"
"    <ul sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"    <lr sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"  </layout>\n"
"  <text/>\n"
"  <installedMethods/>\n"
"  <pushButton id='1' rid='0'>\n"
"    <layout border='0'>\n"
"      <ul sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"      <lr sx='-0.9' sy='-0.9' widget='parent' norm_x='3' norm_y='3'/>\n"
"    </layout>\n"
"    <text align='1' group='1'>b1</text>\n"
"    <installedMethods>\n"
"      <downMethod type='functor'/>\n"
"    </installedMethods>\n"
"  </pushButton>\n"
"  <pushButton id='2' rid='1'>\n"
"    <layout border='0'>\n"
"      <ul sx='-0.9' sy='-0.9' widget='parent' norm_x='3' norm_y='3'/>\n"
"      <lr sx='-0.8' sy='-0.8' widget='parent' norm_x='3' norm_y='3'/>\n"
"    </layout>\n"
"    <text align='2' group='1'>b2</text>\n"
"    <installedMethods>\n"
"      <downMethod type='functor'/>\n"
"    </installedMethods>\n"
"  </pushButton>\n"
"  <pushButton id='3' rid='2'>\n"
"    <layout border='0'>\n"
"      <ul sx='-0.8' sy='-0.8' widget='parent' norm_x='3' norm_y='3'/>\n"
"      <lr sx='-0.7' sy='-0.7' widget='parent' norm_x='3' norm_y='3'/>\n"
"    </layout>\n"
"    <text align='2' group='1'>b3</text>\n"
"    <installedMethods>\n"
"      <downMethod type='functor'/>\n"
"    </installedMethods>\n"
"  </pushButton>\n"
"</panel>\n"s;
    EXPECT_EQ(serialized, os2.str());

    screen.draw(&adapter);
    auto draw =
"<panel id='4' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1000' height='1000' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='50' height='50' border='0'/>\n"
"    <text x='1' y='13' width='48' height='24' charSize='24'/>\n"
"    <string>b1</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='2' rid=1'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='50' y='50' width='50' height='50' border='0'/>\n"
"    <text x='52' y='64' width='46' height='23' charSize='23'/>\n"
"    <string>b2</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='3' rid=2'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='100' y='100' width='50' height='50' border='0'/>\n"
"    <text x='101' y='113' width='48' height='24' charSize='24'/>\n"
"    <string>b3</string>\n"
"  </draw>\n"
"</pushButton>\n"s;
    EXPECT_EQ(draw, os1.str());
}

TEST(Factory, PushButtonGrid)
{
    auto os1        = std::ostringstream{};
    auto adapter    = Draw(os1);
    auto env        = WawtEnv(&adapter);
    auto k1         = int{};
    auto k2         = int{};
    auto k3         = int{};
    auto k4         = int{};

    auto screen
        = panel()
            .addChild(pushButtonGrid({{-0.5,-0.5},{0.5,0.5},2},
                                     1_F,
                                     1,
                                     {
                                         {focusWrap([&k1](auto){k1++;}),
                                          S("k1")}
                                     },
                                     false,
                                     TextAlign::eCENTER));
    screen.assignWidgetIds();
    screen.resizeScreen(1000, 1000);
    EXPECT_EQ(0, k1);
    auto upCb = screen.downEvent(500,500);
    EXPECT_EQ(0, k1);
    EXPECT_FALSE(bool(upCb(500,500,true)));
    EXPECT_EQ(1, k1);
    screen.draw();
    auto draw =
"<panel id='3' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1000' height='1000' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='2' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='250' width='500' height='500' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='250' width='500' height='500' border='2'/>\n"
"    <text x='253' y='377' width='494' height='247' charSize='247'/>\n"
"    <string>k1</string>\n"
"  </draw>\n"
"</pushButton>\n"s;
    EXPECT_EQ(draw, os1.str());
    os1.str("");

    screen
        = panel()
            .addChild(pushButtonGrid({{-0.5,-0.5},{0.5,0.5},2},
                                     1_F,
                                     1,
                                     {
                                         {focusWrap([&k1](auto){k1++;}),
                                          S("k1")},
                                         {focusWrap([&k2](auto){k2++;}),
                                          S("k2")},
                                     },
                                     false,
                                     TextAlign::eCENTER));
    screen.assignWidgetIds();
    screen.resizeScreen(1000, 1000);
    upCb = screen.downEvent(500,500);
    EXPECT_EQ(1, k1);
    EXPECT_EQ(0, k2);
    EXPECT_FALSE(bool(upCb(500,500,true)));
    EXPECT_EQ(1, k1);
    EXPECT_EQ(1, k2);
    screen.draw();
    draw =
"<panel id='4' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1000' height='1000' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='3' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='250' width='500' height='500' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='250' width='500' height='250' border='2'/>\n"
"    <text x='257' y='254' width='486' height='243' charSize='243'/>\n"
"    <string>k1</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='2' rid=1'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='500' width='500' height='250' border='2'/>\n"
"    <text x='257' y='504' width='486' height='243' charSize='243'/>\n"
"    <string>k2</string>\n"
"  </draw>\n"
"</pushButton>\n"s;

    EXPECT_EQ(draw, os1.str());
    os1.str("");

    k1 = k2 = k3 = k4 = 0;
    screen
        = panel()
            .addChild(pushButtonGrid({{-0.5,-0.5},{0.5,0.5},2},
                                     1_F,
                                     2,
                                     {
                                         {focusWrap([&k1](auto){k1++;}),
                                          S("k1")},
                                         {focusWrap([&k2](auto){k2++;}),
                                          S("k2")},
                                         {focusWrap([&k3](auto){k3++;}),
                                          S("k3")},
                                         {focusWrap([&k4](auto){k4++;}),
                                          S("k4")},
                                     },
                                     false,
                                     TextAlign::eCENTER));
    screen.assignWidgetIds();
    screen.resizeScreen(1000, 1000);
    upCb = screen.downEvent(500,500);
    EXPECT_FALSE(bool(upCb(500,500,true)));
    EXPECT_EQ(0, k1);
    EXPECT_EQ(0, k2);
    EXPECT_EQ(0, k3);
    EXPECT_EQ(1, k4);
    screen.draw();
    draw=
"<panel id='6' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1000' height='1000' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='5' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='250' width='500' height='500' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='250' width='250' height='250' border='2'/>\n"
"    <text x='253' y='314' width='244' height='122' charSize='122'/>\n"
"    <string>k1</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='2' rid=1'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='500' y='250' width='250' height='250' border='2'/>\n"
"    <text x='503' y='314' width='244' height='122' charSize='122'/>\n"
"    <string>k2</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='3' rid=2'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='500' width='250' height='250' border='2'/>\n"
"    <text x='253' y='564' width='244' height='122' charSize='122'/>\n"
"    <string>k3</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='4' rid=3'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='500' y='500' width='250' height='250' border='2'/>\n"
"    <text x='503' y='564' width='244' height='122' charSize='122'/>\n"
"    <string>k4</string>\n"
"  </draw>\n"
"</pushButton>\n"s;
    EXPECT_EQ(draw, os1.str());
}

TEST(Factory, SpacedPushButtonGrid)
{
    auto os1        = std::ostringstream{};
    auto adapter    = Draw(os1);
    auto env        = WawtEnv(&adapter);
    auto k1         = int{};
    auto k2         = int{};
    auto k3         = int{};

    auto screen
        = panel({}).addChild(pushButtonGrid({{-1.0,-0.250},{1.0,0.250},2},
                                            1_F,
                                            1,
                                            {
                                             {focusWrap([&k1](auto){k1++;}),
                                              S("*k1")},
                                            },
                                            true,
                                            TextAlign::eCENTER));
    screen.assignWidgetIds();
    screen.resizeScreen(1000, 1000);
    EXPECT_EQ(0, k1);
    auto upCb = screen.downEvent(500,500);
    EXPECT_EQ(0, k1);
    EXPECT_FALSE(bool(upCb(500,500,true)));
    EXPECT_EQ(1, k1);
    screen.draw();
    auto draw =
"<panel id='3' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1000' height='1000' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='2' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='375' width='1000' height='250' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='133' y='376' width='735' height='249' border='2'/>\n"
"    <text x='136' y='379' width='729' height='243' charSize='243'/>\n"
"    <string>*k1</string>\n"
"  </draw>\n"
"</pushButton>\n"s;
    EXPECT_EQ(draw, os1.str());
    os1.str("");

    screen
        = panel({}).addChild(pushButtonGrid({{-1.0,-0.1},{1.0,0.1},2},
                                            1_F,
                                            2,
                                            {
                                             {focusWrap([&k1](auto){k1++;}),
                                              S("*k1")},
                                             {focusWrap([&k2](auto){k2++;}),
                                              S("*k2")},
                                            },
                                            true,
                                            TextAlign::eCENTER));
    screen.assignWidgetIds();
    screen.resizeScreen(1000, 1000);
    EXPECT_FALSE(bool(screen.downEvent(500,500)));
    upCb = screen.downEvent(580,452);
    ASSERT_TRUE(bool(upCb));
    EXPECT_EQ(1, k1);
    EXPECT_EQ(0, k2);
    EXPECT_FALSE(bool(upCb(580,452,true)));
    EXPECT_EQ(1, k1);
    EXPECT_EQ(1, k2);

    screen.draw();
    draw = 
"<panel id='4' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1000' height='1000' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='3' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='450' width='1000' height='100' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='144' y='451' width='285' height='99' border='2'/>\n"
"    <text x='147' y='454' width='279' height='93' charSize='93'/>\n"
"    <string>*k1</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='2' rid=1'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='571' y='451' width='285' height='99' border='2'/>\n"
"    <text x='574' y='454' width='279' height='93' charSize='93'/>\n"
"    <string>*k2</string>\n"
"  </draw>\n"
"</pushButton>\n"s;
    EXPECT_EQ(draw, os1.str());
    os1.str("");

    k1 = k2 = k3 = 0;
    screen
        = panel({}).addChild(pushButtonGrid({{-0.5,-0.5},{0.5,0.5},2},
                                            1_F,
                                            2,
                                            {
                                             {focusWrap([&k1](auto){k1++;}),
                                              S("k1abc")},
                                             {focusWrap([&k2](auto){k2++;}),
                                              S("k2abc")},
                                             {focusWrap([&k3](auto){k3++;}),
                                              S("k3abc")},
                                            },
                                            true,
                                            TextAlign::eCENTER));
    screen.assignWidgetIds();
    screen.resizeScreen(1000, 1000);
    upCb = screen.downEvent(505,450);
    EXPECT_FALSE(bool(upCb(505,450,true)));
    EXPECT_EQ(0, k1);
    EXPECT_EQ(1, k2);
    EXPECT_EQ(0, k3);
    screen.draw();
    draw =
"<panel id='5' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1000' height='1000' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='4' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='250' width='500' height='500' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='1' rid=0'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='433' width='246' height='54' border='2'/>\n"
"    <text x='253' y='436' width='240' height='48' charSize='48'/>\n"
"    <string>k1abc</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='2' rid=1'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='504' y='433' width='246' height='54' border='2'/>\n"
"    <text x='507' y='436' width='240' height='48' charSize='48'/>\n"
"    <string>k2abc</string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='3' rid=2'>\n"
"  <draw options='false' selected='false' disable='false' hidden='false'>\n"
"    <rect x='250' y='514' width='246' height='54' border='2'/>\n"
"    <text x='253' y='517' width='240' height='48' charSize='48'/>\n"
"    <string>k3abc</string>\n"
"  </draw>\n"
"</pushButton>\n"s;
    EXPECT_EQ(draw, os1.str());
}

TEST(Factory, TicTacToe)
{
    using Tuple = WawtEnv::OptionTuple<int>;
    auto defaults = std::array {
        Tuple{ WawtEnv::sScreen, 1.0, 6 },
        Tuple{ WawtEnv::sDialog, 2.0, 5 },
        Tuple{ WawtEnv::sPanel,  3.0, 4 },
        Tuple{ WawtEnv::sLabel,  4.0, 3 },
        Tuple{ WawtEnv::sPush,   5.0, 2 },
        Tuple{ WawtEnv::sBullet, 6.0, 1 }
    };
    std::ostringstream os1;
    Draw    adapter(os1);
    WawtEnv obj(defaults, &adapter);
    Widget *grid;
    int     screenOpt = 7;

    auto click = [](auto *w) { w->resetLabel("X"); return FocusCb(); };

    auto  screen = panelGrid({}, 1, 3, panel({}));
    auto& middle = screen.children()[1];
    middle.addChild( // 0_wr
            panelGrid(&grid,
                      {{-1,-1},{ 1, 1}, Layout::Vertex::eCENTER_CENTER},
                      3,
                      3,
                      pushButton({}, click, S(" "))))
          .addChild( // 1_wr
            panel({{-1,-1,0_wr},{ 1, 1,0_wr}, 5}).options(screenOpt))
          .addChild( // 2_wr - this is a "shim"
            panel({{-1,-1},{-1,-1,0_wr}}))
          .addChild( // 3_wr
            label({{ 1,-1, 2_wr},{ 1,-1,0_wr}}, S("Tic-Tac-Toe")));
    screen.assignWidgetIds();
    screen.resizeScreen(1280, 720);

    screen.draw();
    auto draw =
"<panel id='17' rid=0'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='1280' height='720' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='1' rid=0'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='0' y='0' width='427' height='720' border='3'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='15' rid=1'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='427' y='0' width='427' height='720' border='3'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='11' rid=0'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='430' y='150' width='421' height='421' border='0'/>\n"
"  </draw>\n"
"</panel>\n"
"<pushButton id='2' rid=0'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='430' y='150' width='140' height='140' border='5'/>\n"
"    <text x='436' y='156' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='3' rid=1'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='570' y='150' width='140' height='140' border='5'/>\n"
"    <text x='577' y='156' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='4' rid=2'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='710' y='150' width='140' height='140' border='5'/>\n"
"    <text x='717' y='156' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='5' rid=3'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='430' y='290' width='140' height='140' border='5'/>\n"
"    <text x='436' y='297' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='6' rid=4'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='570' y='290' width='140' height='140' border='5'/>\n"
"    <text x='577' y='297' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='7' rid=5'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='710' y='290' width='140' height='140' border='5'/>\n"
"    <text x='717' y='297' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='8' rid=6'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='430' y='430' width='140' height='140' border='5'/>\n"
"    <text x='436' y='437' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='9' rid=7'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='570' y='430' width='140' height='140' border='5'/>\n"
"    <text x='577' y='437' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<pushButton id='10' rid=8'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='710' y='430' width='140' height='140' border='5'/>\n"
"    <text x='717' y='437' width='127' height='127' charSize='127'/>\n"
"    <string> </string>\n"
"  </draw>\n"
"</pushButton>\n"
"<panel id='12' rid=1'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='430' y='150' width='421' height='421' border='5'/>\n"
"  </draw>\n"
"</panel>\n"
"<panel id='13' rid=2'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='430' y='3' width='0' height='147' border='3'/>\n"
"  </draw>\n"
"</panel>\n"
"<label id='14' rid=3'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='430' y='3' width='421' height='147' border='4'/>\n"
"    <text x='437' y='58' width='407' height='37' charSize='37'/>\n"
"    <string>Tic-Tac-Toe</string>\n"
"  </draw>\n"
"</label>\n"
"<panel id='16' rid=2'>\n"
"  <draw options='true' selected='false' disable='false' hidden='false'>\n"
"    <rect x='853' y='0' width='427' height='720' border='3'/>\n"
"  </draw>\n"
"</panel>\n"s;
    EXPECT_EQ(draw, os1.str());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "fast";
    return RUN_ALL_TESTS();
}

// vim: ts=4:sw=4:et:ai
