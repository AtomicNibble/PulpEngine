#include "stdafx.h"
#include "Id.h"

#include <QByteArray>
#include <QDataStream>
#include <QHash>
#include <QVariant>

X_NAMESPACE_BEGIN(assman)

namespace
{
	class StringHolder
	{
	public:
		StringHolder() :
			n_(0), 
			pStr_(0)
		{}

		StringHolder(const char* pStr, int32_t length) :
			n_(length), 
			pStr_(pStr)
		{
			if (!n_) {
				length = n_ = static_cast<int32_t>(strlen(pStr));
			}
			h_ = 0;
			while (length--) {
				h_ = (h_ << 4) + *pStr++;
				h_ ^= (h_ & 0xf0000000) >> 23;
				h_ &= 0x0fffffff;
			}
		}

	public:
		int32_t n_;
		uint32_t h_;
		const char* pStr_;
	};

	X_INLINE bool operator==(const StringHolder &sh1, const StringHolder &sh2)
	{
		return sh1.h_ == sh2.h_ && sh1.pStr_ && sh2.pStr_ && strcmp(sh1.pStr_, sh2.pStr_) == 0;
	}

	X_INLINE uint qHash(const StringHolder& sh)
	{
		return sh.h_;
	}

	struct IdCache : public QHash<StringHolder, int32_t>
	{
		~IdCache()
		{
			for (IdCache::iterator it = begin(); it != end(); ++it) {
				delete[](const_cast<char*>(it.key().pStr_));
			}
		}
	};

	QHash<int32_t, StringHolder> stringFromId;
	IdCache idFromString;
	int32_t firstUnusedId = 0x1000;

	X_INLINE int32_t theId(const char* pStr, int32_t n = 0)
	{
		StringHolder sh(pStr, n);
		int32_t res = idFromString.value(sh, 0);
		if (res == 0) {
			res = firstUnusedId++;
			sh.pStr_ = qstrdup(sh.pStr_);
			idFromString[sh] = res;
			stringFromId[res] = sh;
		}
		return res;
	}

	X_INLINE int32_t theId(const QByteArray& ba)
	{
		return theId(ba.constData(), ba.size());
	}

} // namespace 


Id::Id(const char *name) : 
	id_(theId(name, 0))
{
}


QByteArray Id::name(void) const
{
	return stringFromId.value(id_).pStr_;
}


QString Id::toString(void) const
{
	return QString::fromUtf8(stringFromId.value(id_).pStr_);
}

QVariant Id::toSetting(void) const
{
	return QVariant(QString::fromUtf8(stringFromId.value(id_).pStr_));
}


Id Id::fromSetting(const QVariant& variant)
{
	const QByteArray ba = variant.toString().toUtf8();
	if (ba.isEmpty()) {
		return Id();
	}
	return Id(theId(ba));
}

Id Id::fromName(const QByteArray &name)
{
	return Id(theId(name));
}


Id Id::withSuffix(int32_t suffix) const
{
	const QByteArray ba = name() + QByteArray::number(suffix);
	return Id(ba.constData());
}

Id Id::withSuffix(const char* pSuffix) const
{
	const QByteArray ba = name() + pSuffix;
	return Id(ba.constData());
}

Id Id::withSuffix(const QString& suffix) const
{
	const QByteArray ba = name() + suffix.toUtf8();
	return Id(ba.constData());
}

Id Id::withPrefix(const char *prefix) const
{
	const QByteArray ba = prefix + name();
	return Id(ba.constData());
}

void Id::registerId(int32_t uid, const char *name)
{
	StringHolder sh(name, 0);
	idFromString[sh] = uid;
	stringFromId[uid] = sh;
}

bool Id::operator==(const char* pName) const
{
	const char *string = stringFromId.value(id_).pStr_;
	if (string && pName) {
		return strcmp(string, pName) == 0;
	}
	else {
		return false;
	}
}


bool Id::alphabeticallyBefore(Id other) const
{
	return toString().compare(other.toString(), Qt::CaseInsensitive) < 0;
}

QString Id::suffixAfter(Id baseId) const
{
	const QByteArray b = baseId.name();
	const QByteArray n = name();
	return n.startsWith(b) ? QString::fromUtf8(n.mid(b.size())) : QString();
}


QDataStream &operator<<(QDataStream &ds, const Id& id)
{
	return ds << id.name();
}

QDataStream &operator >> (QDataStream &ds, Id &id)
{
	QByteArray ba;
	ds >> ba;
	id = Id::fromName(ba);
	return ds;
}


X_NAMESPACE_END