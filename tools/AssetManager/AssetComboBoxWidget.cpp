#include "stdafx.h"
#include "AssetComboBoxWidget.h"

X_NAMESPACE_BEGIN(assman)


AssetComboBoxWidget::AssetComboBoxWidget(QWidget *parent, const std::string& value, bool editable)
	: QComboBox(parent)
{
	X_UNUSED(value);

	setFocusPolicy(Qt::FocusPolicy::StrongFocus);
	setMaxVisibleItems(16);

	QString valueStr = QString::fromStdString(value);
	QStringList values = valueStr.split(QChar('|'));
	QList<int32_t> seperators;

	if (!values.empty())
	{
		// trim he items and look for seperators.
		for (int32_t idx = 0; idx < values.size(); ) 
		{
			QString& val = values[idx];
			val = val.trimmed();

			if (val.count(QChar('-')) == val.length())
			{
				values.removeAt(idx);
				seperators.append(idx);
			}
			else
			{
				++idx;
			}
		}

		addItems(values);

		for (int32_t idx : seperators) {
			insertSeparator(idx);
		}
	}

	if (editable)
	{
		setEditable(1);
		connect(this, SIGNAL(returnPressed(int)), this, SLOT(returnPressed()));
	}

	// cosnt Qstring& version also
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));

}

AssetComboBoxWidget::~AssetComboBoxWidget()
{
}

void AssetComboBoxWidget::currentIndexChanged(int32_t index)
{
	X_UNUSED(index);

}


void AssetComboBoxWidget::returnPressed(void)
{


}

X_NAMESPACE_END