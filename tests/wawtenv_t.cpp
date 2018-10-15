// wawtenv_t.cpp

#include <gtest/gtest.h>
#include <wawt/wawtenv.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <sstream>

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

TEST(Wawt, toString)
{
    EXPECT_EQ(S("0"s),           toString(0));
    EXPECT_EQ(S("2147483647"s),  toString(2147483647));
    EXPECT_EQ(S("-2147483648"s), toString(-2147483648));
}

TEST(Wawt, sizeOfChar)
{
#ifdef WAWT_WIDECHAR
    EXPECT_EQ(sizeof(wchar_t), sizeOfChar(L' '));
#else
    EXPECT_EQ(1, sizeOfChar(U'\u0024'));
    EXPECT_EQ(2, sizeOfChar(U'\u00A2'));
    EXPECT_EQ(3, sizeOfChar(U'\u20AC'));
    EXPECT_EQ(4, sizeOfChar(U'\U00010348'));
    EXPECT_EQ(4, sizeOfChar(U'\U0001f34c'));
#endif
}

TEST(Wawt, Rectangle)
{
    Rectangle r { 11, -12, 333, 421 }; // 11 -12 343 408
    EXPECT_TRUE(r.inside(11,  -12));
    EXPECT_TRUE(r.inside(343, -12));
    EXPECT_TRUE(r.inside(11,  408));
    EXPECT_TRUE(r.inside(343, 408));
    EXPECT_TRUE(r.inside(178, 199));
    EXPECT_FALSE(r.inside(10,  -12));
    EXPECT_FALSE(r.inside(11,  -13));
    EXPECT_FALSE(r.inside(344, -12));
    EXPECT_FALSE(r.inside(343, -13));
    EXPECT_FALSE(r.inside(10,  408));
    EXPECT_FALSE(r.inside(11,  409));
    EXPECT_FALSE(r.inside(344, 408));
    EXPECT_FALSE(r.inside(343, 409));
}

TEST(Wawt, WidgetId)
{
    WidgetId notSet;
    EXPECT_FALSE(notSet.isSet());

    WidgetId absolute(1, false);
    EXPECT_TRUE(absolute.isSet());
    EXPECT_FALSE(absolute.isRelative());

    WidgetId relative(1, true);
    EXPECT_TRUE(relative.isSet());
    EXPECT_TRUE(relative.isRelative());

    EXPECT_TRUE(absolute  == absolute);
    EXPECT_FALSE(absolute == relative);
    EXPECT_FALSE(absolute == notSet);
    EXPECT_FALSE(relative == absolute);
    EXPECT_FALSE(notSet   == absolute);

    EXPECT_TRUE(relative  == relative);
    EXPECT_FALSE(relative == notSet);
    EXPECT_FALSE(notSet   == relative);

    EXPECT_TRUE(notSet    == notSet);

    EXPECT_FALSE(absolute != absolute);
    EXPECT_TRUE(absolute  != relative);
    EXPECT_TRUE(absolute  != notSet);
    EXPECT_TRUE(relative  != absolute);
    EXPECT_TRUE(notSet    != absolute);

    EXPECT_FALSE(relative != relative);
    EXPECT_TRUE(relative  != notSet);
    EXPECT_TRUE(notSet    != relative);

    EXPECT_FALSE(notSet   != notSet);

    WidgetId absolute2(2, false);
    EXPECT_TRUE(absolute < absolute2);
    EXPECT_TRUE(absolute2 > absolute);

    EXPECT_EQ(1, absolute.value());
    EXPECT_EQ(2, absolute2.value());

    EXPECT_TRUE(1_w  == absolute);
    EXPECT_TRUE(1_wr == relative);

    EXPECT_TRUE(WidgetId::kPARENT.isSet());
    EXPECT_TRUE(WidgetId::kPARENT.isRelative());
    EXPECT_TRUE(WidgetId::kPARENT != relative);

    EXPECT_TRUE(WidgetId::kROOT.isSet());
    EXPECT_TRUE(WidgetId::kROOT.isRelative());
    EXPECT_TRUE(WidgetId::kROOT != relative);
}

TEST(Wawt, WawtEnv)
{
    EXPECT_EQ(nullptr, WawtEnv::instance());
    struct X { int y = -1; };
    using Tuple = WawtEnv::OptionTuple<X>;
    auto defaults = std::array{
        Tuple{ WawtEnv::sScreen, 0.0, X{5} },
        Tuple{ WawtEnv::sDialog, 1.0, X{4} },
        Tuple{ WawtEnv::sPanel,  2.0, X{3} },
        Tuple{ WawtEnv::sLabel,  3.0, X{2} },
        Tuple{ WawtEnv::sPush,   4.0, X{1} },
        Tuple{ WawtEnv::sBullet, 5.0, X{0} }
    };
    WawtEnv obj(defaults);

    EXPECT_EQ(&obj, WawtEnv::instance());

    for (auto& [className, thickness, option] : defaults) {
        EXPECT_EQ(thickness,
                  WawtEnv::instance()->defaultBorderThickness(className))
            << className;
        auto any = WawtEnv::instance()->defaultOptions(className);
        EXPECT_STREQ(typeid(X).name(), any.type().name()) << className;
        auto x = X{};
        EXPECT_NO_THROW(x = std::any_cast<X>(any)) << className;
        EXPECT_EQ(option.y, x.y) << className;
    }

    auto any = WawtEnv::instance()->defaultOptions("foobar");
    EXPECT_FALSE(any.has_value());

    EXPECT_EQ(0.0, WawtEnv::instance()->defaultBorderThickness("foobar"s));

    StringView_t ans;
    {
        ans = WawtEnv::instance()->translate(S("abc"s));
    }
    String_t s1(ans);
    EXPECT_EQ(S("abc"s), s1);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "fast";
    return RUN_ALL_TESTS();
}
// vim: ts=4:sw=4:et:ai
