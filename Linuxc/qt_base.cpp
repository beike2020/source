/******************************************************************************
 * Function:   	Qt use.
 * Author:     	forwarding2012@yahoo.com.cn
 * Date:      	2012.01.01
 * Compile:		g++ -Wall qt_base.cpp -o qt_base
******************************************************************************/
#include <qlabel.h>
#include <iostream>
#include <qlayout.h>
#include <qbutton.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qlistview.h>
#include <qcombobox.h>
#include <qmainwindow.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include <kde/kapp.h>
#include <kde/kaction.h>
#include <kde/kstdaccel.h>
#include <kde/kmenubar.h>
#include <kde/kaboutdialog.h>
#include <kde/kmainwindow.h>

#include "LineEdit.moc"
#include "Buttons.moc"
#include "ListView.moc"
#include "ComboBox.moc"

#include "KDEMenu.h"

static int check_model()
{
	int choices;

	printf("Please input test type:\n");
	printf(" 1: show the text use qt\n");
	printf(" 2: show the button use qt\n");
	printf(" 3: show the listview use qt\n");
	printf(" 4: show the combobox use qt\n");
	printf(" 5: show the menu use qt\n");
	sscanf("%d", &choices);

	return choices;
}

class LineEdit:public QMainWindow {
      Q_OBJECT public:
	 LineEdit(QWidget * parent = 0, const char *name = 0);
	QLineEdit *password_entry;

	private slots:void text_Clicked();
};

LineEdit::LineEdit(QWidget * parent, const char *name):QMainWindow(parent, name)
{
	QWidget *widget = new QWidget(this);
	setCentralWidget(widget);
	QGridLayout *grid = new QGridLayout(widget, 3, 2, 10, 10, "grid");

	QLineEdit *username_entry = new QLineEdit(widget, "username_entry");
	password_entry = new QLineEdit(widget, "password_entry");
	password_entry->setEchoMode(QLineEdit::Password);

	grid->addWidget(new QLabel("Username", widget, "userlabel"), 0, 0, 0);
	grid->addWidget(new QLabel("Password", widget, "passwordlabel"), 1, 0,
			0);
	grid->addWidget(username_entry, 0, 1, 0);
	grid->addWidget(password_entry, 1, 1, 0);

	QPushButton *button = new QPushButton("Ok", widget, "button");
	grid->addWidget(button, 2, 1, Qt::AlignRight);

	resize(350, 200);
	connect(button, SIGNAL(clicked()), this, SLOT(text_Clicked()));
}

void LineEdit::text_Clicked(void)
{
	std::cout << password_entry->text() << "\n";
}

int qt_text(int argc, char **argv)
{
	QApplication app(argc, argv);
	LineEdit *window = new LineEdit();

	app.setMainWidget(window);
	window->show();

	return app.exec();
}

class Buttons:public QMainWindow {
      Q_OBJECT public:
	Buttons(QWidget * parent = 0, const char *name = 0);

      private:
	void PrintActive(QButton * button);
	QCheckBox *checkbox;
	QRadioButton *radiobutton1, *radiobutton2;

	private slots:void button_Clicked();

};

Buttons::Buttons(QWidget * parent, const char *name):QMainWindow(parent, name)
{
	QWidget *widget = new QWidget(this);
	setCentralWidget(widget);

	QVBoxLayout *vbox = new QVBoxLayout(widget, 5, 10, "vbox");
	checkbox = new QCheckBox("CheckButton", widget, "check");
	vbox->addWidget(checkbox);
	QButtonGroup *buttongroup = new QButtonGroup(0);

	radiobutton1 = new QRadioButton("RadioButton1", widget, "radio1");
	buttongroup->insert(radiobutton1);
	vbox->addWidget(radiobutton1);

	radiobutton2 = new QRadioButton("RadioButton2", widget, "radio2");
	buttongroup->insert(radiobutton2);
	vbox->addWidget(radiobutton2);

	QPushButton *button = new QPushButton("Ok", widget, "button");
	vbox->addWidget(button);

	resize(350, 200);
	connect(button, SIGNAL(clicked()), this, SLOT(button_Clicked()));
}

void Buttons::PrintActive(QButton * button)
{
	if (button->isOn())
		std::cout << button->name() << " is checked\n";
	else
		std::cout << button->name() << " is not checked\n";
}

void Buttons::button_Clicked(void)
{
	PrintActive(checkbox);
	PrintActive(radiobutton1);
	PrintActive(radiobutton2);
	std::cout << "\n";
}

int qt_button(int argc, char **argv)
{
	QApplication app(argc, argv);
	Buttons *window = new Buttons();

	app.setMainWidget(window);
	window->show();

	return app.exec();
}

class ListView:public QMainWindow {
      Q_OBJECT public:
	ListView(QWidget * parent = 0, const char *name = 0);

      private:
	QListView * listview;

private slots:};

ListView::ListView(QWidget * parent, const char *name):QMainWindow(parent, name)
{
	listview = new QListView(this, "listview1");

	listview->addColumn("Artist");
	listview->addColumn("Title");
	listview->addColumn("Catalogue");
	listview->setRootIsDecorated(TRUE);
	QListViewItem *toplevel =
	    new QListViewItem(listview, "Avril Lavigne", "Let Go", "AVCD01");

	new QListViewItem(toplevel, "Complicated");
	new QListViewItem(toplevel, "Sk8er Boi");
	setCentralWidget(listview);
}

int qt_listview(int argc, char **argv)
{
	QApplication app(argc, argv);
	ListView *window = new ListView();

	app.setMainWidget(window);
	window->show();

	return app.exec();
}

class ComboBox:public QMainWindow {
      Q_OBJECT public:
	 ComboBox(QWidget * parent = 0, const char *name = 0);

	private slots:void box_Changed(const QString & s);
};

ComboBox::ComboBox(QWidget * parent, const char *name):QMainWindow(parent, name)
{
	QWidget *widget = new QWidget(this);
	setCentralWidget(widget);

	QVBoxLayout *vbox = new QVBoxLayout(widget, 5, 10, "vbox");
	QComboBox *editablecombo = new QComboBox(TRUE, widget, "editable");
	vbox->addWidget(editablecombo);
	QComboBox *readonlycombo = new QComboBox(FALSE, widget, "readonly");
	vbox->addWidget(readonlycombo);

	static const char *items[] =
	    { "Macbeth", "Twelfth Night", "Othello", 0 };
	editablecombo->insertStrList(items);
	readonlycombo->insertStrList(items);

	connect(editablecombo, SIGNAL(textChanged(const QString &)),
		this, SLOT(Changed(const QString &)));
	resize(350, 200);
}

void ComboBox::box_Changed(const QString & s)
{
	std::cout << s << "\n";
}

int qt_combobox(int argc, char **argv)
{
	QApplication app(argc, argv);
	ComboBox *window = new ComboBox();

	app.setMainWidget(window);
	window->show();

	return app.exec();
}

class KDEMenu:public KMainWindow {
      Q_OBJECT public:
	KDEMenu(const char *name);

	private slots:void newFile();
	void aboutApp();
};

KDEMenu::KDEMenu(const char *name):KMainWindow(0L, name)
{
	KAction *act =
	    new KAction("&New", "filenew", KStdAccel::shortcut(KStdAccel::New),
			this,
			SLOT(newFile()), this);
	KAction *act2 =
	    KStdAction::quit(KApplication::kApplication(), SLOT(quit()),
			     actionCollection());
	KAction *help_action =
	    KStdAction::aboutApp(this, SLOT(aboutApp()), actionCollection());
	QPopupMenu *file_menu = new QPopupMenu;
	QPopupMenu *help_menu = new QPopupMenu;

	menuBar()->insertItem("&File", file_menu);
	menuBar()->insertItem("&Help", help_menu);

	act->plug(file_menu);
	file_menu->insertSeparator();
	act2->plug(file_menu);
	help_action->plug(help_menu);

	act->plug(toolBar());
	act2->plug(toolBar());
}

void KDEMenu::newFile()
{

}

void KDEMenu::aboutApp()
{
	KAboutDialog *about = new KAboutDialog(this, "dialog");
	about->setAuthor(QString("A. N. Author"), QString("an@email.net"),
			 QString("http://url.com"), QString("work"));
	about->setVersion("1.0");

	about->show();
}

int qt_menu(int argc, char **argv)
{
	KApplication app(argc, argv, "cdapp");
	KDEMenu *window = new KDEMenu("bob");

	app.setMainWidget(window);
	window->show();

	return app.exec();
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//show the text use qt
	case 1:
		qt_text(argc, argv);
		break;

		//show the button use qt
	case 2:
		qt_button(argc, argv);
		break;

		//show the listview use qt
	case 3:
		qt_listview(argc, argv);
		break;

		//show the combobox use qt
	case 4:
		qt_combobox(argc, argv);
		break;

		//show the menu use qt
	case 5:
		qt_menu(argc, argv);
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
