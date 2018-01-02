#pragma once

#include <QObject>
#include <QWidget>
#include <QPointer>

#include "Id.h"

X_NAMESPACE_BEGIN(assman)


class Context
{
public:
	typedef QList<Id>::const_iterator const_iterator;

public:
	Context() = default;

	X_INLINE explicit Context(Id c1);
	X_INLINE Context(Id c1, Id c2);
	X_INLINE Context(Id c1, Id c2, Id c3);

	X_INLINE bool contains(Id c) const;
	X_INLINE int32_t size(void) const;
	X_INLINE bool isEmpty(void) const;
	X_INLINE Id at(int32_t idx) const;


	X_INLINE int32_t indexOf(Id c) const;
	X_INLINE void removeAt(int32_t idx);
	X_INLINE void prepend(Id c);
	X_INLINE void add(const Context& c);
	X_INLINE void add(Id c);

	X_INLINE bool operator==(const Context& c) const;

	X_INLINE const_iterator begin(void) const;
	X_INLINE const_iterator end(void) const;

private:
	QList<Id> ids_;
};


class IContext : public QObject
{
	Q_OBJECT
public:
	X_INLINE IContext(QObject* pParent = nullptr);

	X_INLINE virtual Context context(void) const;
	X_INLINE virtual QWidget* widget(void) const;

	X_INLINE virtual void setContext(const Context& context);
	X_INLINE virtual void setWidget(QWidget* pWidget);

protected:
	Context context_;
	QPointer<QWidget> widget_;
};


X_NAMESPACE_END

#include "Context.inl"