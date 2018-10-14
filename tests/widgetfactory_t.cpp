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

const char s_TicTacToe[] =
u8"<panel id='17' rid='0'>\n"
"  <layout border='0'>\n"
"    <ul sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"    <lr sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"  </layout>\n"
"  <text/>\n"
"  <installedMethods/>\n"
"  <panel id='1' rid='0'>\n"
"    <layout border='3'>\n"
"      <ul sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"      <lr sx='-0.333333' sy='1' widget='parent' norm_x='3' norm_y='3'/>\n"
"    </layout>\n"
"    <text/>\n"
"    <installedMethods/>\n"
"  </panel>\n"
"  <panel id='15' rid='1'>\n"
"    <layout border='3'>\n"
"      <ul sx='1' sy='-1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"      <lr sx='3' sy='1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"    </layout>\n"
"    <text/>\n"
"    <installedMethods/>\n"
"    <panel id='11' rid='0'>\n"
"      <layout border='0' pin='4'>\n"
"        <ul sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"        <lr sx='1' sy='1' widget='parent' norm_x='3' norm_y='3'/>\n"
"      </layout>\n"
"      <text/>\n"
"      <installedMethods/>\n"
"      <pushButton id='2' rid='0'>\n"
"        <layout border='5'>\n"
"          <ul sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"          <lr sx='-0.333333' sy='-0.333333' widget='parent' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='127' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='3' rid='1'>\n"
"        <layout border='5'>\n"
"          <ul sx='1' sy='-1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='3' sy='1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='127' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='4' rid='2'>\n"
"        <layout border='5'>\n"
"          <ul sx='1' sy='-1' widget='1_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='3' sy='1' widget='1_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='127' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='5' rid='3'>\n"
"        <layout border='5'>\n"
"          <ul sx='-1' sy='1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='1' sy='2' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='57' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='6' rid='4'>\n"
"        <layout border='5'>\n"
"          <ul sx='1' sy='-1' widget='3_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='3' sy='1' widget='3_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='57' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='7' rid='5'>\n"
"        <layout border='5'>\n"
"          <ul sx='1' sy='-1' widget='4_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='3' sy='1' widget='4_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='57' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='8' rid='6'>\n"
"        <layout border='5'>\n"
"          <ul sx='-1' sy='1' widget='3_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='1' sy='2' widget='3_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='22' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='9' rid='7'>\n"
"        <layout border='5'>\n"
"          <ul sx='1' sy='-1' widget='6_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='3' sy='1' widget='6_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='22' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"      <pushButton id='10' rid='8'>\n"
"        <layout border='5'>\n"
"          <ul sx='1' sy='-1' widget='7_wr' norm_x='3' norm_y='3'/>\n"
"          <lr sx='3' sy='1' widget='7_wr' norm_x='3' norm_y='3'/>\n"
"        </layout>\n"
"        <text align='2' charSize='22' group=''> </text>\n"
"        <installedMethods>\n"
"          <downMethod type='functor'/>\n"
"        </installedMethods>\n"
"      </pushButton>\n"
"    </panel>\n"
"    <panel id='12' rid='1'>\n"
"      <layout border='5'>\n"
"        <ul sx='-1' sy='-1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"        <lr sx='1' sy='1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"      </layout>\n"
"      <text/>\n"
"      <installedMethods/>\n"
"    </panel>\n"
"    <panel id='13' rid='2'>\n"
"      <layout border='3'>\n"
"        <ul sx='-1' sy='-1' widget='parent' norm_x='3' norm_y='3'/>\n"
"        <lr sx='-1' sy='-1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"      </layout>\n"
"      <text/>\n"
"      <installedMethods/>\n"
"    </panel>\n"
"    <label id='14' rid='3'>\n"
"      <layout border='4'>\n"
"        <ul sx='1' sy='-1' widget='2_wr' norm_x='3' norm_y='3'/>\n"
"        <lr sx='1' sy='-1' widget='0_wr' norm_x='3' norm_y='3'/>\n"
"      </layout>\n"
"      <text align='2' charSize='37' group=''>Tic-Tac-Toe</text>\n"
"      <installedMethods/>\n"
"    </label>\n"
"  </panel>\n"
"  <panel id='16' rid='2'>\n"
"    <layout border='3'>\n"
"      <ul sx='1' sy='-1' widget='1_wr' norm_x='3' norm_y='3'/>\n"
"      <lr sx='3' sy='1' widget='1_wr' norm_x='3' norm_y='3'/>\n"
"    </layout>\n"
"    <text/>\n"
"    <installedMethods/>\n"
"  </panel>\n"
"</panel>\n";

TEST(Factory, TicTacToe)
{
    using Tuple = WawtEnv::OptionTuple<int>;
    auto defaults = std::array {
        Tuple{ WawtEnv::sScreen, 1.0, 6 },
        Tuple{ WawtEnv::sDialog, 2.0, 5 },
        Tuple{ WawtEnv::sPanel,  3.0, 4 },
        Tuple{ WawtEnv::sLabel,  4.0, 3 },
        Tuple{ WawtEnv::sPush,   5.0, 2 },
        Tuple{ WawtEnv::sCheck,  6.0, 1 },
    };
    Draw    adapter;
    WawtEnv obj(defaults, &adapter);
    Widget *grid;
    int     screenOpt = 7;

    auto click = [](auto *w) { w->resetLabel("X"); };

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
    std::ostringstream os;
    screen.serialize(os);
    EXPECT_EQ(0, os.str().compare(s_TicTacToe));
    screen.draw();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "fast";
    return RUN_ALL_TESTS();
}

// vim: ts=4:sw=4:et:ai
