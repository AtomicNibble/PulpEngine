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
		QString title;
		QString value;
	};

	typedef QList<ComboEntry> ComboEntryArr;


public:
	AssetComboBoxWidget(QWidget *parent, const std::string& value, const std::string& valuesStr, bool editable = false);
	~AssetComboBoxWidget();


public:
	static bool splitValues(const std::string& values, ComboEntryArr& valuesOut, QList<int32_t>* pSeperators = nullptr);

signals:
	void valueChanged(const std::string& value);

private slots:
	void setValue(const std::string& value);
	void currentIndexChanged(int index);
	void returnPressed(void);

private:
	void wheelEvent(QWheelEvent *e);

private:
	ComboEntryArr values_;
	bool editable_;
};



X_NAMESPACE_END