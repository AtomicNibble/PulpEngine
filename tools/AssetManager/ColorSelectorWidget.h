#pragma once

#include <QObject>

#include "ColorPreviewWidget.h"

X_NAMESPACE_BEGIN(assman)


class ColorSelector : public ColorPreview
{
	Q_OBJECT

public:
	enum class UpdateMode {
		Confirm, ///< Update color only after the dialog has been accepted
		Continuous ///< Update color as it's being modified in the dialog
	};

public:
	explicit ColorSelector(QWidget *parent = 0);
	~ColorSelector();

	void setUpdateMode(UpdateMode m);
	UpdateMode updateMode(void) const;

	Qt::WindowModality dialogModality(void) const;
	void setDialogModality(Qt::WindowModality m);

signals:
	void colorSelected(const QColor& col);

public slots:
	void showDialog(void);

private slots:
	void rejectDialog(void);
	void dialogColorSelected(const QColor& color);

protected:
	void dragEnterEvent(QDragEnterEvent* event);
	void dropEvent(QDropEvent* event);

private:
	void connectDialog(void);
	void disconnectDialog(void);

private:
	class Private;
	Private* const p;
};

X_NAMESPACE_END