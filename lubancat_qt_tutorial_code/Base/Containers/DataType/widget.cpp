#include "widget.h"
#include "ui_widget.h"

Widget * Widget::m_widget = nullptr;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    m_widget = this;
    qInstallMessageHandler(logOutput);  //重定向qdebug输出

    test_bitArray();
    test_byteArray();
    test_string();
    test_stringList();
 }


/**************************************************************
 *
 *                      QBitArray
 *
 * ************************************************************/

void Widget::test_bitArray()
{

    qDebug()<<"####################QBitArray操作####################";
    // 初始化200位默认false（0）的位
    QBitArray bit(200);
    qDebug()<<"初始化200位bit: "<<bit;

    // 索引位操作
    bit.resize(3);
    bit[0] = true;
    bit[1] = false;
    bit[2] = true;
    qDebug()<<bit;

    //使用 testBit() 和 setBit()位操作
    bit.setBit(0, false);
    bit.setBit(1, true);
    bit.setBit(2, false);
    qDebug()<<bit;

    qDebug()<<QBitArray().isNull();
    qDebug()<<QBitArray().isEmpty();
    qDebug()<<QBitArray(3).isNull();
    qDebug()<<QBitArray(3).isEmpty();

    // 位运算
    QBitArray bit_x(5);
    bit_x.setBit(3, true);// x: [ 0, 0, 0, 1, 0 ]

    QBitArray bit_y(5);
    bit_y.setBit(4, true);// y: [ 0, 0, 0, 0, 1 ]

    bit_x |= bit_y;// x: [ 0, 0, 0, 1, 1 ]
    qDebug()<<bit_x<<bit_y;
    bit_x &= bit_y;// x: [ 0, 0, 0, 0, 1 ]
    qDebug()<<bit_x<<bit_y;

    qDebug()<<"";
}

/**************************************************************
 *
 *                      QByteArray
 *
 * ************************************************************/
void Widget::test_byteArray()
{
    qDebug()<<"################QByteArray操作#######################";
    QByteArray byte("Hello");
    qDebug()<<"byte数据: "<<byte.data();
    qDebug()<<"byte大小: "<<byte.size();

    byte.resize(5);
    byte[0] = 0x41;
    byte[1] = 0x42;
    byte[2] = 0x43;
    byte[3] = 0x44;
    byte[4] = 0x45;
    qDebug()<<"";
    qDebug()<<"重新初始化byte: "<<byte.data();
    qDebug()<<"输出byte[3]: "<<byte[3];    //这是字符输出，0x44对应大写字母D
    qDebug()<<"输出byte[4]: "<<byte[4];

    qDebug()<<"";
    qDebug()<<"输出byte每个字节:";
    for (int i = 0; i < byte.size();i++) {
        qDebug()<<byte.at(i);
    }

    qDebug()<<"";
    qDebug()<<"输出byte.right(2): "<<byte.right(2);
    qDebug()<<"输出byte.left(2): "<<byte.left(2);
    qDebug()<<"输出byte.mid(2): "<<byte.mid(2,2);

    QByteArray byte1("ca\0r\0t");
    qDebug()<<"";
    qDebug()<<"byte1.data(): "<<byte1.data();
    qDebug()<<"输出byte1大小: "<<byte1.size();                              // Returns 2.
    qDebug()<<"输出byte1.constData(): "<<byte1.constData();                // Returns "ca" with terminating \0.

    QByteArray byte2("ca\0r\0t", 3);
    qDebug()<<byte2.size();                     // Returns 3.
    qDebug()<<byte2.constData();                // Returns "ca\0" with terminating \0.

    QByteArray byte3("ca\0r\0t", 4);
    qDebug()<<byte3.size();                     // Returns 4.
    qDebug()<<byte3.constData();                // Returns "ca\0r" with terminating \0.
    for(int i=0; i<byte3.size();i++)
    {
        qDebug()<<byte3.at(i);
    }

    const char cart[] = {'c', 'a', '\0', 'r', '\0', 't'};
    QByteArray byte4(QByteArray::fromRawData(cart, 6));
    qDebug()<<byte4.size();                     // Returns 6.
    qDebug()<<byte4.constData();                // Returns "ca\0r\0t" without terminating \0.

    QByteArray byte5("and");
    qDebug()<<"";
    qDebug()<<"修改字节数据的基本功能(插入,添加等)："<<byte.right(2);
    qDebug()<<byte5;
    byte5.prepend("rock ");
    qDebug()<<byte5;
    byte5.append(" roll");
    qDebug()<<byte5;
    byte5.replace(5, 3, "&");
    qDebug()<<byte5;

    QByteArray byte6("We must be <b>bold</b>, very <b>bold</b>");
    int j = 0;
    while ((j = byte6.indexOf("<b>", j)) != -1) {
        qDebug() << "Found <b> tag at index position " << j;
        ++j;
    }

    if(byte6.contains("must"))
    {
        qDebug()<<byte6.count("b");
    }

    qDebug()<<QByteArray().isNull();
    qDebug()<<QByteArray().isEmpty();
    qDebug()<<QByteArray("").isNull();
    qDebug()<<QByteArray("").isEmpty();
    qDebug()<<QByteArray("abc").isNull();
    qDebug()<<QByteArray("abc").isEmpty();

    QByteArray byte7("0f");
    bool ok;
    qDebug()<<"";
    qDebug()<<"大小写转换:";
    qDebug()<<byte7.toInt(&ok,10);
    qDebug()<<byte7.toInt(&ok,16);
    qDebug()<<byte7.toLong(&ok,10);
    qDebug()<<byte7.toUpper();
    qDebug()<<"";
}


/**************************************************************
 *
 *                      QString
 *
 * ************************************************************/
void Widget::test_string()
{
    qDebug()<<"####################QString操作####################";

    QString str = "Hello";
    static const QChar data[4] = {0x0055, 0x006e, 0x10e3, 0x03a3};
    QString str1(data, 4);

    QString str2;
    str2.resize(4);

    str[0] = QChar('U');
    str[1] = QChar('n');
    str[2] = QChar(0x10e3);
    str[3] = QChar(0x03a3);

    QString str3="asfcfrvfbgkiocmewiocmiq";
    for (int i = 0; i<str.size(); i++)
    {
        if (str3.at(i) >= QChar('a')  && str3.at(i) <= QChar('f'))
        qDebug()  << "输出范围在[a-f]的字符:"<<str3.at(i);
    }

    // 比较两个字符串
    QString str4="extern";
    if (str4 == "auto")
        qDebug()<<"str is auto";
    else if(str4 == "extern")
        qDebug()<<"str is extern";
    else
        qDebug()<<str4;

   // 字符串的添加，替换等
    QString str5("and");
    qDebug()<<str5;
    str5.prepend("rock ");
    qDebug()<<str5;
    str5.append(" roll");
    qDebug()<<str5;
    str5.replace(5, 3, "&");
    qDebug()<<str5;

    // 字符串的截取，大小写等
    qDebug()<<str5.toUpper();
    qDebug()<<str5.toLower();
    qDebug()<<str5.count("l");
    qDebug()<<str5.length();
    qDebug()<<str5.left(5).size();
    qDebug()<<str5.mid(2,8);
    qDebug()<<str5.append(str5);

    QString str6 = "We must be <b>bold</b>, very <b>bold</b>";
    int j=0;
    while ((j = str6.indexOf("<b>", j)) != -1) {
        qDebug()  << "Found <b> tag at index position:"<<j;
        ++j;
    }

    // QString和数值之间的转换：
    QString str7 = "56.5";
    qDebug()<<str7.toInt();
    qDebug()<<str7.toDouble();
    qDebug()<<str7.toFloat();
    qDebug()<<str7.toLong();
    qDebug()<<QString::number(53);
    qDebug()<<QString::number(4.4156);
    qDebug()<<QString::number(4.4156,'f',2);

    QString str8 = "F0F0";
    bool OK;
    int val = str8.toInt(&OK,16);
    qDebug()<<val;
    qDebug()<<QString::number(val,2);
    qDebug()<<QString::number(val,8);
    qDebug()<<QString::number(val,10);
    qDebug()<<QString::number(val,16);

    qDebug()<<str8.toLocal8Bit();
    qDebug()<<str8.toLatin1();
    qDebug()<<str8.toUtf8();

    QString str9 = "To get an upper - or lowercase version of a string use toUpper() or toLower().";
    QStringList list= str9.split(" ");
    int count=0;
    for(int i=0; i<list.count();i++)
    {
        qDebug()<<list.at(i);
        if(list.at(i)== "or")
        {
            qDebug()<< "'or' times:"<<++count;
        }
    }

    QString name ="xiao fang";
    int age=22;
    qDebug()<<QString("%1 is %2 years old").arg(name).arg(age);

    qDebug()<<"";
}

/**************************************************************
 *
 *                      QStringList
 *
 * ************************************************************/

void Widget::test_stringList()
{

    qDebug()<<"####################QStringList操作####################";

    QStringList fonts = { "Arial", "Helvetica", "Times" };
    fonts << "Courier" << "Verdana";
    fonts += "chinese";

    // 遍历QStringList
    for (int i = 0; i < fonts.size(); ++i)
             qDebug()<< fonts.at(i).toLocal8Bit().constData();

    QStringListIterator javaStyleIterator(fonts);
        while (javaStyleIterator.hasNext())
            qDebug() << javaStyleIterator.next().toLocal8Bit().constData();

    QStringList::const_iterator constIterator;
        for (constIterator = fonts.constBegin(); constIterator != fonts.constEnd();++constIterator)
            qDebug() << (*constIterator).toLocal8Bit().constData();

    // 字符串列表中的所有字符串连接一个
    QString str_list = fonts.join(", ");// str == "Arial, Helvetica, Times, Courier"   //单个字符串
    qDebug() <<  str_list;

    //分离
    QStringList list_font = str_list.split(',');// list: ["Arial", "Helvetica", "Times", "Courier"]
    qDebug() <<  list_font;
    for (int i = 0; i < list_font.size(); ++i)
        qDebug() <<  list_font.at(i);

    //匹配字符串
    QStringList monospacedFonts = fonts.filter(QRegularExpression("Courier|Fixed"));
    qDebug() <<  monospacedFonts;

    QStringList files;
    files << "$QTDIR/src/moc/moc.y"
          << "$QTDIR/src/moc/moc.l"
          << "$QTDIR/include/qconfig.h";

    //遍历
    for (int i = 0; i < files.size(); ++i)
        qDebug() <<  files.at(i);

    files.replaceInStrings("$QTDIR", "/usr/lib/qt");    //replaceInStrings()函数对字符串列表中的每个字符串进行replace操作
    for (int i = 0; i < files.size(); ++i)
        qDebug() <<  files.at(i);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::logOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Widget::write(msg);
}

void Widget::write(QString str)
{
    m_widget->ui->plainTextEdit->appendPlainText(str);
}

