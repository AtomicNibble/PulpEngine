X_NAMESPACE_BEGIN(assman)

#include <limits.h>

namespace Utils
{


	QString settingsKey(const QString &category)
	{
		QString rc(category);
		const QChar underscore = QLatin1Char('_');
		// Remove the sort category "X.Category" -> "Category"
		if (rc.size() > 2 && rc.at(0).isLetter() && rc.at(1) == QLatin1Char('.'))
			rc.remove(0, 2);
		// Replace special characters
		const int size = rc.size();
		for (int i = 0; i < size; i++) {
			const QChar c = rc.at(i);
			if (!c.isLetterOrNumber() && c != underscore)
				rc[i] = underscore;
		}
		return rc;
	}

	// Figure out length of common start of string ("C:\a", "c:\b"  -> "c:\"
	static inline int commonPartSize(const QString &s1, const QString &s2)
	{
		const int size = qMin(s1.size(), s2.size());
		for (int i = 0; i < size; i++)
			if (s1.at(i) != s2.at(i))
				return i;
		return size;
	}

	QString commonPrefix(const QStringList &strings)
	{
		switch (strings.size()) {
		case 0:
			return QString();
		case 1:
			return strings.front();
		default:
			break;
		}
		// Figure out common string part: "C:\foo\bar1" "C:\foo\bar2"  -> "C:\foo\bar"
		int commonLength = INT_MAX;
		const int last = strings.size() - 1;
		for (int i = 0; i < last; i++)
			commonLength = qMin(commonLength, commonPartSize(strings.at(i), strings.at(i + 1)));
		if (!commonLength)
			return QString();
		return strings.at(0).left(commonLength);
	}

	QString commonPath(const QStringList &files)
	{
		QString common = commonPrefix(files);
		// Find common directory part: "C:\foo\bar" -> "C:\foo"
		int lastSeparatorPos = common.lastIndexOf(QLatin1Char('/'));
		if (lastSeparatorPos == -1)
			lastSeparatorPos = common.lastIndexOf(QLatin1Char('\\'));
		if (lastSeparatorPos == -1)
			return QString();
		//    if (HostOsInfo::isAnyUnixHost() && lastSeparatorPos == 0) // Unix: "/a", "/b" -> '/'
		//        lastSeparatorPos = 1;
		common.truncate(lastSeparatorPos);
		return common;
	}


} // namespace Utils



X_NAMESPACE_END