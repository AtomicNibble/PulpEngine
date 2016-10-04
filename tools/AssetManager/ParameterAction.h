#pragma once

#include <QObject>


X_NAMESPACE_BEGIN(assman)

class ParameterAction : public QAction
{
	Q_OBJECT
public:
	enum class EnablingMode { 
		AlwaysEnabled,
		EnabledWithParameter 
	};

public:
	explicit ParameterAction(const QString &emptyText,
		const QString &parameterText,
		EnablingMode em = EnablingMode::AlwaysEnabled,
		QObject *parent = nullptr);

	QString emptyText(void) const;
	void setEmptyText(const QString &);

	QString parameterText(void) const;
	void setParameterText(const QString &);

	EnablingMode enablingMode(void) const;
	void setEnablingMode(EnablingMode m);

public slots:
	void setParameter(const QString &);

private:
	QString emptyText_;
	QString parameterText_;
	EnablingMode enablingMode_;
};



X_NAMESPACE_END