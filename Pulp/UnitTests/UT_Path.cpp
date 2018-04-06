#include "stdafx.h"

X_USING_NAMESPACE;

TEST(Path, Construct)
{
    core::Path<char> path;

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    EXPECT_EQ(260, path.capacity());
    EXPECT_EQ(0, path.length());

    path.ensureSlash();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.replaceSeprators();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.removeFileName();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.removeExtension();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.removeTrailingSlash();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());
}

TEST(Path, ConstructW)
{
    core::Path<wchar_t> path;

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    EXPECT_EQ(260, path.capacity());
    EXPECT_EQ(0, path.length());

    path.ensureSlash();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.replaceSeprators();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.removeFileName();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.removeExtension();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());

    path.removeTrailingSlash();

    EXPECT_TRUE(path.isEmpty());
    EXPECT_FALSE(path.isNotEmpty());
    EXPECT_FALSE(path.isAbsolute());
}

TEST(Path, Construct2)
{
    {
        core::Path<char> path("");

        EXPECT_TRUE(path.isEmpty());
        EXPECT_FALSE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(0, path.length());
        EXPECT_STREQ("", path.c_str());
    }
    {
        core::Path<char> path("camel");

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(5, path.length());
        EXPECT_STREQ("camel", path.c_str());
    }
    {
        core::Path<char> path("c:\\o_my\\goat");

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_TRUE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(12, path.length());
        EXPECT_STREQ("c:\\o_my\\goat", path.c_str());
    }
}

TEST(Path, Construct2W)
{
    {
        core::Path<wchar_t> path(L"");

        EXPECT_TRUE(path.isEmpty());
        EXPECT_FALSE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(0, path.length());
        EXPECT_STREQ(L"", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"camel");

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(5, path.length());
        EXPECT_STREQ(L"camel", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\o_my\\goat");

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_TRUE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(12, path.length());
        EXPECT_STREQ(L"c:\\o_my\\goat", path.c_str());
    }
}

TEST(Path, ConstructConvert)
{
    {
        core::Path<wchar_t> pathW(L"");
        core::Path<char> path(pathW);

        EXPECT_TRUE(path.isEmpty());
        EXPECT_FALSE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(0, path.length());
        EXPECT_STREQ("", path.c_str());
    }
    {
        core::Path<wchar_t> pathW(L"camel");
        core::Path<char> path(pathW);

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(5, path.length());
        EXPECT_STREQ("camel", path.c_str());
    }
    {
        core::Path<wchar_t> pathW(L"c:\\o_my\\goat");
        core::Path<char> path(pathW);

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_TRUE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(12, path.length());
        EXPECT_STREQ("c:\\o_my\\goat", path.c_str());
    }
}

TEST(Path, ConstructConvertW)
{
    {
        core::Path<char> pathN("");
        core::Path<wchar_t> path(pathN);

        EXPECT_TRUE(path.isEmpty());
        EXPECT_FALSE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(0, path.length());
        EXPECT_STREQ(L"", path.c_str());
    }
    {
        core::Path<char> pathN("camel");
        core::Path<wchar_t> path(pathN);

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_FALSE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(5, path.length());
        EXPECT_STREQ(L"camel", path.c_str());
    }
    {
        core::Path<char> pathN("c:\\o_my\\goat");
        core::Path<wchar_t> path(pathN);

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());
        EXPECT_TRUE(path.isAbsolute());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(12, path.length());
        EXPECT_STREQ(L"c:\\o_my\\goat", path.c_str());
    }
}

TEST(Path, Copy)
{
    core::Path<char> path1;
    core::Path<char> path2("goat");

    {
        core::Path<char> path(path1);

        EXPECT_TRUE(path.isEmpty());
        EXPECT_FALSE(path.isNotEmpty());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(0, path.length());
    }
    {
        core::Path<char> path(path2);

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(4, path.length());

        EXPECT_STREQ("goat", path.c_str());
    }
}

TEST(Path, CopyW)
{
    core::Path<wchar_t> path1;
    core::Path<wchar_t> path2(L"goat");

    {
        core::Path<wchar_t> path(path1);

        EXPECT_TRUE(path.isEmpty());
        EXPECT_FALSE(path.isNotEmpty());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(0, path.length());
    }
    {
        core::Path<wchar_t> path(path2);

        EXPECT_FALSE(path.isEmpty());
        EXPECT_TRUE(path.isNotEmpty());

        EXPECT_EQ(260, path.capacity());
        EXPECT_EQ(4, path.length());

        EXPECT_STREQ(L"goat", path.c_str());
    }
}

TEST(Path, FileName)
{
    {
        core::Path<char> path("why\\do\\goats\\smell.so.good");

        EXPECT_STREQ("smell.so.good", path.fileName());
    }
    {
        core::Path<char> path("why\\do\\goats\\");

        EXPECT_STREQ("", path.fileName());
    }
    {
        core::Path<char> path("why\\do\\goats\\smell.so.good/cat");

        EXPECT_STREQ("cat", path.fileName());
    }
}

TEST(Path, FileNameW)
{
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.so.good");

        EXPECT_STREQ(L"smell.so.good", path.fileName());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\");

        EXPECT_STREQ(L"", path.fileName());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.so.good/cat");

        EXPECT_STREQ(L"cat", path.fileName());
    }
}

TEST(Path, Extension)
{
    core::Path<char> path("why\\do\\goats\\smell.so.good");

    EXPECT_STREQ(".good", path.extension());
}

TEST(Path, ExtensionW)
{
    core::Path<wchar_t> path(L"why\\do\\goats\\smell.so.good");

    EXPECT_STREQ(L".good", path.extension());
}

TEST(Path, setExtension)
{
    {
        core::Path<char> path("why\\do\\goats\\smell.so.good");
        path.setExtension("");
        EXPECT_STREQ("why\\do\\goats\\smell.so", path.c_str());
    }
    {
        core::Path<char> path("why\\do\\goats\\");
        path.setExtension(".cat");
        EXPECT_STREQ("why\\do\\goats\\.cat", path.c_str());
    }
    {
        core::Path<char> path("why\\do\\goats\\smell.");
        path.setExtension("sexy");
        EXPECT_STREQ("why\\do\\goats\\smell.sexy", path.c_str());
    }
    {
        core::Path<char> path("why\\do\\goats\\smell.");
        path.setExtension(".sexy");
        EXPECT_STREQ("why\\do\\goats\\smell.sexy", path.c_str());
    }
    {
        core::Path<char> path("why\\do\\goats\\smell.");
        path.setExtension(".sexy");
        EXPECT_STREQ("why\\do\\goats\\smell.sexy", path.c_str());
    }
}

TEST(Path, setExtensionW)
{
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.so.good");
        path.setExtension(L"");
        EXPECT_STREQ(L"why\\do\\goats\\smell.so", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\");
        path.setExtension(L".cat");
        EXPECT_STREQ(L"why\\do\\goats\\.cat", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.");
        path.setExtension(L"sexy");
        EXPECT_STREQ(L"why\\do\\goats\\smell.sexy", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.");
        path.setExtension(L".sexy");
        EXPECT_STREQ(L"why\\do\\goats\\smell.sexy", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell");
        path.setExtension(L".sexy");
        EXPECT_STREQ(L"why\\do\\goats\\smell.sexy", path.c_str());
    }
}

TEST(Path, setFileName)
{
    {
        core::Path<char> path("why\\do\\goats\\smell.so.good");
        path.setFileName("");
        EXPECT_STREQ("why\\do\\goats\\", path.c_str());
    }
    {
        core::Path<char> path("why\\do\\goats\\");
        path.setFileName("fat_cat");
        EXPECT_STREQ("why\\do\\goats\\fat_cat", path.c_str());
    }
    {
        core::Path<char> path("why\\do\\goats\\smell.");
        path.setFileName("sezy.pickle");
        EXPECT_STREQ("why\\do\\goats\\sezy.pickle", path.c_str());
    }
    {
        core::Path<char> path("why\\do\\goats\\smell.");
        path.setFileName("BigGoat");
        EXPECT_STREQ("why\\do\\goats\\BigGoat", path.c_str());
    }
}

TEST(Path, setFileNameW)
{
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.so.good");
        path.setFileName(L"");
        EXPECT_STREQ(L"why\\do\\goats\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\");
        path.setFileName(L"fat_cat");
        EXPECT_STREQ(L"why\\do\\goats\\fat_cat", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.");
        path.setFileName(L"sezy.pickle");
        EXPECT_STREQ(L"why\\do\\goats\\sezy.pickle", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"why\\do\\goats\\smell.");
        path.setFileName(L"BigGoat");
        EXPECT_STREQ(L"why\\do\\goats\\BigGoat", path.c_str());
    }
}

TEST(Path, Assign)
{
    core::Path<char> path("why\\do\\goats\\smell.so.good");

    EXPECT_STREQ("why\\do\\goats\\smell.so.good", path.c_str());

    path = "tickle_me_plz";

    EXPECT_STREQ("tickle_me_plz", path.c_str());
}

TEST(Path, AssignW)
{
    core::Path<wchar_t> path(L"why\\do\\goats\\smell.so.good");

    EXPECT_STREQ(L"why\\do\\goats\\smell.so.good", path.c_str());

    path = L"tickle_me_plz";

    EXPECT_STREQ(L"tickle_me_plz", path.c_str());
}

TEST(Path, Append)
{
    core::Path<char> path("c:\\");
    EXPECT_STREQ("c:\\", path.c_str());
    path = path + "tickle_me_plz";
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path = path + "\\goat\\";
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\", path.c_str());
}
TEST(Path, AppendW)
{
    core::Path<wchar_t> path(L"c:\\");
    EXPECT_STREQ(L"c:\\", path.c_str());
    path = path + L"tickle_me_plz";
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path = path + L"\\goat\\";
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\", path.c_str());
}
TEST(Path, AppendPath)
{
    core::Path<char> path("c:\\");
    EXPECT_STREQ("c:\\", path.c_str());
    path = path + core::Path<char>("tickle_me_plz");
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path = path + core::Path<char>("\\goat\\");
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\", path.c_str());
}
TEST(Path, AppendPathW)
{
    core::Path<wchar_t> path(L"c:\\");
    EXPECT_STREQ(L"c:\\", path.c_str());
    path = path + core::Path<wchar_t>(L"tickle_me_plz");
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path = path + core::Path<wchar_t>(L"\\goat\\");
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\", path.c_str());
}
TEST(Path, AppendAssign)
{
    core::Path<char> path("c:\\");
    EXPECT_STREQ("c:\\", path.c_str());
    path += "tickle_me_plz";
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path += "\\goat\\";
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\", path.c_str());
}
TEST(Path, AppendAssignW)
{
    core::Path<wchar_t> path(L"c:\\");
    EXPECT_STREQ(L"c:\\", path.c_str());
    path += L"tickle_me_plz";
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path += L"\\goat\\";
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\", path.c_str());
}
TEST(Path, AppendPathAssign)
{
    core::Path<char> path("c:\\");
    EXPECT_STREQ("c:\\", path.c_str());
    path += core::Path<char>("tickle_me_plz");
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path += core::Path<char>("\\goat\\");
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\", path.c_str());
}
TEST(Path, AppendPathAssignW)
{
    core::Path<wchar_t> path(L"c:\\");
    EXPECT_STREQ(L"c:\\", path.c_str());
    path += core::Path<wchar_t>(L"tickle_me_plz");
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path += core::Path<wchar_t>(L"\\goat\\");
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\", path.c_str());
}

TEST(Path, Concat)
{
    core::Path<char> path("c:\\");

    EXPECT_STREQ("c:\\", path.c_str());
    path = path / "tickle_me_plz";
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path = path / "goat\\";
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\", path.c_str());
}

TEST(Path, ConcatW)
{
    core::Path<wchar_t> path(L"c:\\");

    EXPECT_STREQ(L"c:\\", path.c_str());
    path = path / L"tickle_me_plz";
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path = path / L"goat\\";
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\", path.c_str());
}

TEST(Path, ConcatPath)
{
    core::Path<char> path("c:\\");

    EXPECT_STREQ("c:\\", path.c_str());
    path = path / core::Path<char>("tickle_me_plz");
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path = path / core::Path<char>("goat\\");
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\", path.c_str());
}

TEST(Path, ConcatPathW)
{
    core::Path<wchar_t> path(L"c:\\");

    EXPECT_STREQ(L"c:\\", path.c_str());
    path = path / core::Path<wchar_t>(L"tickle_me_plz");
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path = path / core::Path<wchar_t>(L"goat\\");
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\", path.c_str());
}

TEST(Path, ConcatAssign)
{
    core::Path<char> path("c:\\");

    EXPECT_STREQ("c:\\", path.c_str());
    path /= "tickle_me_plz";
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path /= "goat";
    EXPECT_STREQ("c:\\tickle_me_plz\\goat", path.c_str());
    path /= "goat\\"; // should auto add slash.
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\goat\\", path.c_str());
}

TEST(Path, ConcatAssignW)
{
    core::Path<wchar_t> path(L"c:\\");

    EXPECT_STREQ(L"c:\\", path.c_str());
    path /= L"tickle_me_plz";
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path /= L"goat";
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat", path.c_str());
    path /= L"goat\\"; // should auto add slash.
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\goat\\", path.c_str());
}

TEST(Path, ConcatPathAssign)
{
    core::Path<char> path("c:\\");

    EXPECT_STREQ("c:\\", path.c_str());
    path /= core::Path<char>("tickle_me_plz");
    EXPECT_STREQ("c:\\tickle_me_plz", path.c_str());
    path /= core::Path<char>("goat");
    EXPECT_STREQ("c:\\tickle_me_plz\\goat", path.c_str());
    path /= core::Path<char>("goat\\");
    EXPECT_STREQ("c:\\tickle_me_plz\\goat\\goat\\", path.c_str());
}

TEST(Path, ConcatPathAssignW)
{
    core::Path<wchar_t> path(L"c:\\");

    EXPECT_STREQ(L"c:\\", path.c_str());
    path /= core::Path<wchar_t>(L"tickle_me_plz");
    EXPECT_STREQ(L"c:\\tickle_me_plz", path.c_str());
    path /= core::Path<wchar_t>(L"goat\\");
    EXPECT_STREQ(L"c:\\tickle_me_plz\\goat\\", path.c_str());
}

TEST(Path, EncureSlash)
{
    {
        core::Path<char> path("c:\\");
        path.ensureSlash();
        EXPECT_STREQ("c:\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat");
        path.ensureSlash();
        EXPECT_STREQ("c:\\goat\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat/");
        path.ensureSlash();
        EXPECT_STREQ("c:\\goat/\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat.");
        path.ensureSlash();
        EXPECT_STREQ("c:\\goat.\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat.ext");
        path.ensureSlash();
        EXPECT_STREQ("c:\\goat.ext\\", path.c_str());
    }
}

TEST(Path, EncureSlashW)
{
    {
        core::Path<wchar_t> path(L"c:\\");
        path.ensureSlash();
        EXPECT_STREQ(L"c:\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat");
        path.ensureSlash();
        EXPECT_STREQ(L"c:\\goat\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat/");
        path.ensureSlash();
        EXPECT_STREQ(L"c:\\goat/\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat.");
        path.ensureSlash();
        EXPECT_STREQ(L"c:\\goat.\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat.ext");
        path.ensureSlash();
        EXPECT_STREQ(L"c:\\goat.ext\\", path.c_str());
    }
}

TEST(Path, replaceSeprators)
{
    {
        core::Path<char> path("c:\\pickle/\\goat\\");
        path.replaceSeprators();
        EXPECT_STREQ("c:\\pickle\\\\goat\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\pickle/goat");
        path.replaceSeprators();
        EXPECT_STREQ("c:\\pickle\\goat", path.c_str());
    }
}

TEST(Path, replaceSepratorsW)
{
    {
        core::Path<wchar_t> path(L"c:\\pickle/\\goat\\");
        path.replaceSeprators();
        EXPECT_STREQ(L"c:\\pickle\\\\goat\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\pickle/goat");
        path.replaceSeprators();
        EXPECT_STREQ(L"c:\\pickle\\goat", path.c_str());
    }
}

TEST(Path, RemoveFileName)
{
    {
        core::Path<char> path("c:\\");
        path.removeFileName();
        EXPECT_STREQ("c:\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat");
        path.removeFileName();
        EXPECT_STREQ("c:\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat/");
        path.removeFileName();
        EXPECT_STREQ("c:\\goat/", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat.");
        path.removeFileName();
        EXPECT_STREQ("c:\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat.ext");
        path.removeFileName();
        EXPECT_STREQ("c:\\", path.c_str());
    }
}

TEST(Path, RemoveFileNameW)
{
    {
        core::Path<wchar_t> path(L"c:\\");
        path.removeFileName();
        EXPECT_STREQ(L"c:\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat");
        path.removeFileName();
        EXPECT_STREQ(L"c:\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat/");
        path.removeFileName();
        EXPECT_STREQ(L"c:\\goat/", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat.");
        path.removeFileName();
        EXPECT_STREQ(L"c:\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat.ext");
        path.removeFileName();
        EXPECT_STREQ(L"c:\\", path.c_str());
    }
}

TEST(Path, RemoveExtension)
{
    {
        core::Path<char> path("c:\\");
        path.removeExtension();
        EXPECT_STREQ("c:\\", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat");
        path.removeExtension();
        EXPECT_STREQ("c:\\goat", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat/");
        path.removeExtension();
        EXPECT_STREQ("c:\\goat/", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat.");
        path.removeExtension();
        EXPECT_STREQ("c:\\goat", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat.ext");
        path.removeExtension();
        EXPECT_STREQ("c:\\goat", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat.ext.cat");
        path.removeExtension();
        EXPECT_STREQ("c:\\goat.ext", path.c_str());
    }
}

TEST(Path, RemoveExtensionW)
{
    {
        core::Path<wchar_t> path(L"c:\\");
        path.removeExtension();
        EXPECT_STREQ(L"c:\\", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat");
        path.removeExtension();
        EXPECT_STREQ(L"c:\\goat", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat/");
        path.removeExtension();
        EXPECT_STREQ(L"c:\\goat/", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat.");
        path.removeExtension();
        EXPECT_STREQ(L"c:\\goat", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat.ext");
        path.removeExtension();
        EXPECT_STREQ(L"c:\\goat", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat.ext.cat");
        path.removeExtension();
        EXPECT_STREQ(L"c:\\goat.ext", path.c_str());
    }
}

TEST(Path, RemoveTrailingSlash)
{
    {
        core::Path<char> path("c:\\");
        path.removeTrailingSlash();
        EXPECT_STREQ("c:", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat\\");
        path.removeTrailingSlash();
        EXPECT_STREQ("c:\\goat", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat/");
        path.removeTrailingSlash();
        EXPECT_STREQ("c:\\goat", path.c_str());
    }
    {
        core::Path<char> path("c:\\goat");
        path.removeTrailingSlash();
        EXPECT_STREQ("c:\\goat", path.c_str());
    }
}

TEST(Path, RemoveTrailingSlashW)
{
    {
        core::Path<wchar_t> path(L"c:\\");
        path.removeTrailingSlash();
        EXPECT_STREQ(L"c:", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat\\");
        path.removeTrailingSlash();
        EXPECT_STREQ(L"c:\\goat", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat/");
        path.removeTrailingSlash();
        EXPECT_STREQ(L"c:\\goat", path.c_str());
    }
    {
        core::Path<wchar_t> path(L"c:\\goat");
        path.removeTrailingSlash();
        EXPECT_STREQ(L"c:\\goat", path.c_str());
    }
}