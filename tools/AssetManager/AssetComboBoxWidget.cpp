#include "stdafx.h"
#include "AssetComboBoxWidget.h"

X_NAMESPACE_BEGIN(assman)


AssetComboBoxWidget::AssetComboBoxWidget(QWidget *parent, const std::string& value, const std::string& valuesStr, bool editable) :
	QComboBox(parent),
	editable_(editable)
{
	setFocusPolicy(Qt::FocusPolicy::StrongFocus);
	setMaxVisibleItems(16);

	QList<int32_t> seperators;

	splitValues(valuesStr, values_, &seperators);

	for (const auto& val : values_) {
		addItem(val.title);
	}

	for (int32_t idx : seperators) {
		insertSeparator(idx);
	}

	if (editable)
	{
		setEditable(1);
		QLineEdit* pLineEdit = lineEdit();
		if (pLineEdit) {
			connect(pLineEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
		}
	}

	// cosnt Qstring& version also
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)));

	setFocusPolicy(Qt::StrongFocus);

	// we need to map values.
	setValue(value);
}

AssetComboBoxWidget::~AssetComboBoxWidget()
{
}


void AssetComboBoxWidget::setValue(const std::string& value)
{
	blockSignals(true);

	QString str = QString::fromStdString(value);

	// support mapping the property back to it's title so we can select by index.
	for (const auto& val : values_) {
		if (val.value == str) {
			str = val.title;
			break;
		}
	}

	if (editable_)
	{
		setCurrentText(str);
	}
	else
	{
		// should be part of the collection.
		// if not we have a invalid value.
		int32_t idx = findText(str);
		if(idx == -1)
		{
			// item not fnd, tut tut.
			addItem(str);
			idx = findText(str);

			setItemIcon(idx, QIcon(":/misc/img/warning_32.png"));
			setItemData(idx, QColor(Qt::red), Qt::TextColorRole);
		}

		setCurrentIndex(idx);
	}

	blockSignals(false);
}

bool AssetComboBoxWidget::splitValues(const std::string& valuesStd, ComboEntryArr& valuesOut, QList<int32_t>* pSeperators)
{
	QString valueStr = QString::fromStdString(valuesStd);
	QStringList values = valueStr.split(QChar('|'));

	QRegExp overrideReg("(\\[|\\])", Qt::CaseSensitivity::CaseSensitive, QRegExp::PatternSyntax::RegExp);
	QStringList parts;

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

				if (pSeperators) {
					pSeperators->append(idx);
				}
			}
			else
			{
				ComboEntry entry;

				// check if we have a value override.
				parts = val.split(overrideReg, QString::SplitBehavior::SkipEmptyParts);
				if (parts.size() == 2)
				{
					entry.title = parts[0].trimmed();
					entry.value = parts[1].trimmed();
				}
				else
				{
					entry.title = val;
					entry.value = val;
				}

				valuesOut.append(entry);

				++idx;
			}
		}
	}

	return true;
}


void AssetComboBoxWidget::currentIndexChanged(int index)
{
	if (index >= values_.size()) {
		X_ERROR("Combo", "Index out of range %i -> %i",index, values_.size());
		return;
	}

	const auto& entry = values_[index];

	QString val = entry.value;

	emit valueChanged(val.toStdString());
}


void AssetComboBoxWidget::returnPressed(void)
{


}

void AssetComboBoxWidget::wheelEvent(QWheelEvent *e)
{
	if (hasFocus()) {
		QComboBox::wheelEvent(e);
	}
}

X_NAMESPACE_END