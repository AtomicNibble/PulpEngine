#pragma once

#include <QObject>

X_NAMESPACE_BEGIN(assman)


class Id
{
public:
	X_INLINE Id();
	X_INLINE Id(const Id& oth);
	X_INLINE Id(int32_t uid);
	Id(const char* pName);

	Id withSuffix(int32_t suffix) const;
	Id withSuffix(const char* pSuffix) const;
	Id withSuffix(const QString& suffix) const;
	Id withPrefix(const char* pPrefix) const;

	QByteArray name(void) const;
	QString toString(void) const; // Avoid.
	QVariant toSetting(void) const; // Good to use.
	QString suffixAfter(Id baseId) const;
	X_INLINE bool isValid(void) const;
	X_INLINE bool operator==(Id id) const;
	bool operator==(const char* pName) const;
	X_INLINE bool operator!=(Id id) const;
	X_INLINE bool operator!=(const char* pName) const;
	X_INLINE bool operator<(Id id) const;
	X_INLINE bool operator>(Id id) const;
	bool alphabeticallyBefore(Id other) const;
	X_INLINE int32_t uniqueIdentifier(void) const;


	static X_INLINE Id fromUniqueIdentifier(int32_t uid);
	static Id fromSetting(const QVariant& variant); // Good to use.
	static Id fromName(const QByteArray &ba);
	static void registerId(int32_t uid, const char* pName);

private:
	// Intentionally unimplemented
	Id(const QLatin1String &);
	int32_t id_;
};

static X_INLINE uint qHash(const Id& id) 
{ 
	return id.uniqueIdentifier();
}

QDataStream &operator<<(QDataStream &ds, const Id &id);
QDataStream &operator >> (QDataStream &ds, Id &id);

X_NAMESPACE_END



#include "Id.inl"