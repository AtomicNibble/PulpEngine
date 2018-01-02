#include "stdafx.h"
#include "AddModDialog.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(assman)



AddModDialog::AddModDialog(QWidget *parent, assetDb::AssetDB& db) :
	QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
	db_(db)
{
	setWindowTitle("Add Mod");

	QFormLayout* pFormLayout = new QFormLayout();

	{
		pModName_ = new QLineEdit();

		QRegularExpression re(QString("[a-z0-9_\\%1]*").arg(QChar(assetDb::ASSET_NAME_SLASH)));

		pModName_->setMaxLength(assetDb::ASSET_NAME_MAX_LENGTH);
		pModName_->setValidator(new QRegularExpressionValidator(re));

		pFormLayout->addRow("Name", pModName_);
	}

	{
		pOutDir_ = new QLineEdit();

		pFormLayout->addRow("Dir", pOutDir_);
	}

	{
		QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		pButtonBox->button(QDialogButtonBox::Ok)->setDefault(true);
		connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
		connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
		pFormLayout->addWidget(pButtonBox);
	}

	setLayout(pFormLayout);
}

AddModDialog::~AddModDialog()
{
}

void AddModDialog::accept(void)
{
	core::string modName;
	core::Path<char> ourDir;

	// name
	{
		QString name = pModName_->text();

		if (name.isEmpty()) {
			QMessageBox::critical(this, "Invalid mod name", "Mod name is empty", QMessageBox::Ok);
			return;
		}

		const auto latinStr = name.toLatin1();
		modName = latinStr;
	}
	// dir
	{
		QString dir = pOutDir_->text();

		if (dir.isEmpty()) {
			QMessageBox::critical(this, "Invalid mod dir", "Mod directory is empty", QMessageBox::Ok);
			return;
		}

		// validate the path?
		// I kinda want the paths to be game folder relative.
		// so too do this correct we need to know game folder and do some custom selection widget
		// and then validate.
		// not gonna bother for now, since no noobs will be using this for a long time.

		const auto latinStr = dir.toLatin1();
		ourDir = latinStr;
	}

	if (db_.ModExsists(modName)) {
		QMessageBox::critical(this, "Add Mod", "A mod with this name already exsists", QMessageBox::Ok);
		return;
	}

	auto res = db_.AddMod(modName, ourDir);
	if (res == assetDb::AssetDB::Result::OK) {
		done(QDialog::Accepted);
		return;
	}

	QString msg = QString("Error adding mod. Error: %1").arg(QString::number(res));
	QMessageBox::critical(this, "Add Mod", msg, QMessageBox::Ok);
}

void AddModDialog::reject(void)
{
	done(QDialog::Rejected);
}


void AddModDialog::done(int32_t val)
{
	QDialog::done(val);
}


X_NAMESPACE_END