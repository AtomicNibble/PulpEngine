#pragma once
#include <QString>


X_NAMESPACE_BEGIN(editor)

namespace Utils
{

	// Create a usable settings key from a category,
	// for example Editor|C++ -> Editor_C__
	QString settingsKey(const QString &category);

	// Return the common prefix part of a string list:
	// "C:\foo\bar1" "C:\foo\bar2"  -> "C:\foo\bar"
	QString commonPrefix(const QStringList &strings);

	// Return the common path of a list of files:
	// "C:\foo\bar1" "C:\foo\bar2"  -> "C:\foo"
	QString commonPath(const QStringList &files);



	X_INLINE QString capitalize(const QString& str)
	{
		if (str.isEmpty()) {
			return str;
		}

		QString tmp = str.toLower();
		tmp[0] = str[0].toUpper();
		return tmp;
	}

} // namespace Utils



X_NAMESPACE_END