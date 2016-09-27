#pragma once

#include <QObject>
#include <QComboBox>


X_NAMESPACE_BEGIN(assman)


class AssetComboBoxWidget : public QComboBox
{
	Q_OBJECT

public:
	struct ComboEntry
	{
		X_INLINE bool hasOverRide(void) const;

		QString title;
		QString valueOverride;
	};

	typedef QList<ComboEntry> ComboEntryArr;


public:
	AssetComboBoxWidget(QWidget *parent, const std::string& value, const std::string& valuesStr, bool editable = false);
	~AssetComboBoxWidget();


public:
	static bool splitValues(const std::string& values, ComboEntryArr& valuesOut, QList<int32_t>* pSeperators = nullptr);

private:
	void setValue(const std::string& value);

private slots:
	void currentIndexChanged(int index);
	void returnPressed(void);

private:
	ComboEntryArr values_;
	bool editable_;
};

X_INLINE bool AssetComboBoxWidget::ComboEntry::hasOverRide(void) const
{
	return !valueOverride.isEmpty();
}


X_NAMESPACE_END