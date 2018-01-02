#include "stdafx.h"
#include "AssetStringWidget.h"


X_NAMESPACE_BEGIN(editor)



StringValidator::StringValidator(QObject *parent) :
	QValidator(parent)
{

}


StringValidator::State StringValidator::validate(QString& input, int& pos) const
{
	X_UNUSED(pos);

	if (input.isEmpty()) {
		return Acceptable;
	}

	QRegExp rx("[\r\n\t]");
	if (input.contains(rx)) {
		input = input.replace(rx, 0);
	}

	return Acceptable;
}

// ---------------------------------------------------------------------------------------


AssetStringWidget::AssetStringWidget(QWidget *parent, const std::string& value) :
	QLineEdit(parent)
{
	// removes any new line.
	setValidator(new StringValidator());
	setToolTip("String value, linebreaks are removed");

	setValue(value);

	connect(this, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
}

AssetStringWidget::~AssetStringWidget()
{
}

void AssetStringWidget::setValue(const std::string& value)
{
	blockSignals(true);
	setText(QString::fromStdString(value));
	blockSignals(false);
}


void AssetStringWidget::editingFinished(void)
{
	QString val = text();

	emit valueChanged(val.toStdString());
}

X_NAMESPACE_END