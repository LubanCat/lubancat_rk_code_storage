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
	
    // 测试各种容器，取消注释，进行测试
    test_qlist();
    test_qvector();
//    test_qstack();
//    test_queue();
//    test_qset();
//    test_qmap();
//    test_qmultimap();
//    test_hash();
//    test_qmultihash();
//    test_iteratorJava();
//    test_iteratorStl();
//    test_foreach();
}

/**************************************************************
 *
 *                      QList
 *
 * ************************************************************/
void Widget::test_qlist()
{
    qDebug() << "################ QList操作 ##################";
    // QList<T> 定义一个元素类型是T的列表,定义时可以初始化列表数据和大小

    // 创建一个整数列表:
    QList<int> integerList ={1,2,3,4,5,6,7,8,9};

    // 创建一个字符列表:
    QList<QChar> dateList;

    // 创建一个字符串列表
    QList<QString> list = {"one", "two", "three"};

    // 将字符串 "four" 和 "five" 添加到字符串列表list末尾
    list << "four" << "five";

    // 在字符串列表尾部添加元素
    list.append("six");

    // 在字符串列表头部添加元素
    list.prepend("zero");

    // 如果字符串列表list的第一个元素是 "one"，将其改为 "Robert"
    if (list[0] == "one")
        list[0] = "Robert";

    // 遍历字符串列表list，打印输出
    for (int i = 0; i< list.size(); ++i) {
            qDebug() << list.at(i);;
    }

    // 获取 "three" 在字符串列表list中第一次出现的位置
    int index = list.indexOf("three");
    if (index != -1)
       qDebug() << "第一次出现three的位置： " << index << Qt::endl;
}

/**************************************************************
 *
 *                      QVector
 *
 * ************************************************************/

void Widget::test_qvector()
{
    qDebug() << "################ QVector简单操作 ################";
    // 创建一个整数向量,可以看成整型数组
    QVector<int> integerVector={1,2,3,4,5,6,7,8,9};

    // 创建一个长度为10的字符串向量
    QVector<QString> vector1(10);
    // 创建一个长度为10的字符串向量，初始值为"Pass"
    QVector<QString> vector2(10, "Pass");

    // 创建一个字符串向量vector，如果向量的第一个元素是"zero"，将其改为"one"
    QVector<QString> vector = {"zero", "two", "three"};
    if (vector[0] == "zero")
        vector[0] = "one";

    // 遍历向量
    for (int i = 0; i< vector.size();++i) {
            qDebug() << vector.at(i);
    }

    // 获取"two"第一次出现的位置
    int index = vector.indexOf("two");
    if (index != -1)
        qDebug() << "第一次出现two的位置是: " << index << Qt::endl;
}

/**************************************************************
 *
 *                      QStack
 *
 * ************************************************************/
void Widget::test_qstack()
{
    qDebug() << "################ QStack简单操作 ################";

    // 创建一个整数类型的栈（QStack）对象，名为stack
    QStack<int> stack;

    // 使用push()压入元素
    stack.push(1);
    stack.push(2);
    stack.push(3);

    // 使用top()查看栈顶元素
    qDebug() << "Top element:" << stack.top();

    // 使用size()查看堆栈大小
    qDebug() << "Stack size:" << stack.size();

    // 使用pop()移除并返回栈顶元素
    int poppedValue = stack.pop();
    qDebug() << "Popped value:" << poppedValue;

    // 使用isEmpty()检查堆栈是否为空
    qDebug() << "堆栈是否空的:" << (stack.isEmpty() ? "Yes" : "No");

    // 使用operator<<压入元素
    stack << 4 << 5;

    // 当栈不为空时执行循环，输出并弹出栈顶的元素
    while (!stack.isEmpty())
       qDebug() << stack.pop();

    qDebug() << "堆栈是否空的:" << (stack.isEmpty() ? "Yes" : "No") << Qt::endl;
}

/**************************************************************
*
*                      QQueue
*
* ************************************************************/
void Widget::test_queue()
{
    qDebug() << "################ QQueue简单操作 ################";

    // 创建一个空的int型 QQueue对象，名为queue
    QQueue<int> queue;

    // 入队
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);

    // 获取队列大小
    qDebug() << "队列大小:" << queue.size();

    // 检查队列是否为空
    qDebug() << "队列是否为空:" << queue.isEmpty();

    // 查看队列的头部元素
    qDebug() << "队列的头部元素:" << queue.head();

    // 出队元素
    int firstElement = queue.dequeue();
    qDebug() << "出队元素:" << firstElement;

    // 获取队列中的第一个和最后一个元素
    qDebug() << "First element:" << queue.first();
    qDebug() << "Last element:" << queue.last();

    // 检查队列是否包含特定值
    qDebug() << "检查队列是否包含特定值 3:" << queue.contains(3);

    // 出队所有元素
    while (!queue.isEmpty())
       qDebug() << queue.dequeue();

    qDebug() << "";
}

/**************************************************************
 *
 *                      QSet
 *
 * ************************************************************/
void Widget::test_qset()
{
    qDebug() << "################ QQueue简单操作 ################";

    // 创建一个字符串类型的集合（QSet）对象，名为set
    QSet<QString> set;

    // 向集合中插入字符串"one" 、"three"、"seven"
    set.insert("one");
    set.insert("three");
    set.insert("seven");

    // 使用流操作符连续插入字符串"twelve"、"fifteen"、"nineteen"到集合中
    set  << "twelve" << "fifteen" << "nineteen";

    // 判断集合中是否包含字符串"fifteen"，如果包含则输出"set contains fifteen"
    if (set.contains("fifteen"))
        qDebug()<<"set contains fifteen";

    // 使用foreach循环遍历集合中的字符串，并输出其值
    foreach (const QString &value, set)
        qDebug()  << value;

    qDebug() << "";
}

/**************************************************************
 *
 *                      QMap
 *
 * ************************************************************/
void Widget::test_qmap()
{
    qDebug() << "################ QMap简单操作 ################";

    // 创建一个空的QString到int的映射（QMap)
    QMap<QString, int> map;
    map["one"] = 1;        //在映射中插入或修改键为"one"的元素，其值为1
    map["three"] = 3;      //在映射中插入或修改键为"three"的元素，其值为3
    map["seven"] = 7;      //在映射中插入或修改键为"seven"的元素，其值为7
    map.insert("twelve", 12);  //使用insert()函数在映射中插入或修改键为"twelve"的元素，其值为12

    //使用[],从映射中获取键为"thirteen"的元素的值，并输出
    int num1 = map["three"];
    qDebug()<<num1;

    //使用value()从映射中获取键为"thirteen"的元素的值，并输出
    int num2 = map.value("three");
    qDebug()<<num2;

    // 使用contains()函数检查映射中是否存在键为"twelve"的元素。如果存在则执行下面的代码块，将timeout的值设置为映射中键为"twelve"的元素的值
    int timeout = 30;
    if (map.contains("twelve"))
        timeout = map.value("twelve");
    qDebug()<< "timeout = " << timeout;

    // 创建一个QMapIterator对象mapIterator_java，用于遍历映射map，输出所有元素的键和值。
    QMapIterator<QString, int> mapIterator_java(map);
    while (mapIterator_java.hasNext()) {
        mapIterator_java.next();
        qDebug() << mapIterator_java.key() << ": " << mapIterator_java.value();
    }

    // 创建一个const_iterator常量迭代器，用也于遍历映射map
    QMap<QString, int>::const_iterator mapIterator_stl = map.constBegin();
    while (mapIterator_stl != map.constEnd()) {
        qDebug() << mapIterator_stl.key() << ": " << mapIterator_stl.value();
        ++mapIterator_stl;
    }

    // 创建一个迭代器i，用于遍历映射map，找到three元素为止
    QMap<QString, int>::iterator i = map.find("three");
    while (i != map.end() && i.key() == "three") {
        qDebug() << i.value();
        ++i;
    }

    // 使用foreach循环遍历映射中的每个值
    foreach (int value, map)
        qDebug() << value;

    qDebug() << "";
}

/**************************************************************
 *
 *                      QMultiMap
 *
 * ************************************************************/
void Widget::test_qmultimap()
{
    qDebug() << "################ QMultiMap简单操作 ################";

    // 声明三个QMultiMap类型的变量，QMultiMap是一种可以存储键值对的容器，并且可以存储重复的键。
    QMultiMap<QString, int> map1, map2, map3;

    map1.insert("plenty", 100);    // 在map1中插入键为"plenty"，值为100的元素
    map1.insert("plenty", 2000);   // 在map1中再次插入键为"plenty"，值为2000的元素，这不会覆盖之前的元素，而是在相同键下插入新的元素
    qDebug()<<map1.size();

    map2.insert("plenty", 5000);  // 在map2中插入键为"plenty"，值为5000的元素。
    qDebug()<<map2.size();

    // 使用QMultiMap的operator+将map1和map2合并，结果存储在map3中
    // 如果两个map中存在相同的键，那么在map3中该键对应的值将是两个map中该键对应的所有值的集合
    map3 = map1 + map2;
    qDebug()<<map3.size();

    // 获取在map3中键为"plenty"的所有值，并将这些值存储到列表中
    // 然后遍历valueslist，并输出其中的每个值
    QList<int> valueslist = map3.values("plenty");
    for (int i = 0; i< valueslist.size(); ++i)
        qDebug() << valueslist.at(i);

    qDebug() << "";
}

/**************************************************************
 *
 *                      QHash
 *
 * ************************************************************/
void Widget::test_hash()
{
    qDebug() << "################ QHash操作 ##################";

    QHash<QString, int> hash;
    hash["one"] = 1;
    hash["three"] = 3;
    hash["seven"] = 7;
    hash.insert("twelve", 12);
    qDebug() << hash["thirteen"];
    qDebug() << hash.value("thirteen");

    if (hash.contains("twelve"))
        qDebug() << hash.value("twelve");

    qDebug() << hash.value("twelve", 30);

    QHashIterator<QString, int> hashIterator_java(hash);
    while (hashIterator_java.hasNext()) {
        hashIterator_java.next();
        qDebug() << hashIterator_java.key() << ": " << hashIterator_java.value();
    }

    QHash<QString, int>::const_iterator hashIterator_stl = hash.constBegin();
    while (hashIterator_stl != hash.constEnd()) {
        qDebug() << hashIterator_stl.key() << ": " << hashIterator_stl.value();
        ++hashIterator_stl;
    }

    foreach (int value, hash)
        qDebug() << value;

    qDebug() << "";
}

/**************************************************************
 *
 *                      QMultiHash
 *
 * ************************************************************/
void Widget::test_qmultihash()
{
    qDebug() << "################ QMultiHash操作 ##################";

    QMultiHash<QString, int> hash1, hash2, hash3;

    hash1.insert("plenty", 100);
    hash1.insert("plenty", 2000);
    qDebug() << hash1.size();

    hash2.insert("plenty", 5000);
    qDebug() << hash2.size();

    hash3 = hash1 + hash2;
    qDebug() << hash3.size();

    QList<int> hashvalues = hash1.values("plenty");
    for (int i = 0; i < hashvalues.size(); ++i)
        qDebug() << hashvalues.at(i);

    qDebug() << "";
}

/**************************************************************
 *
 *                      java迭代器
 *
 * ************************************************************/
void Widget::test_iteratorJava()
{
    qDebug() << "################java迭代器################";
    QList<QString> listiterator;
    listiterator << "A" << "B" << "C" << "D";

    // 遍历
    QListIterator<QString> listiterator_java1(listiterator);
    while (listiterator_java1.hasNext())
        qDebug() << listiterator_java1.next();
    qDebug() << "";

    //向前迭代
    QListIterator<QString> listiterator_java2(listiterator);
    listiterator_java2.toBack();
    while (listiterator_java2.hasPrevious())
        qDebug() << listiterator_java2.previous();
    qDebug() << "";

    QList<int> mutableiterator;
    mutableiterator << 1 << 2 << 3 << 4;

    // 删除不能被2整除的
    QMutableListIterator<int> mutableiterator_java1(mutableiterator);
    while (mutableiterator_java1.hasNext()) {
        if (mutableiterator_java1.next() % 2 != 0)
            mutableiterator_java1.remove();
    }
    while (mutableiterator_java1.hasPrevious())
        qDebug() << mutableiterator_java1.previous();
    qDebug() << "";

    QList<int> mutableiterator1;
    mutableiterator1 << 129 << 1 << 150 << 4;

    // 修改小于128的值
    QMutableListIterator<int> mutableiterator_java3(mutableiterator1);
    while (mutableiterator_java3.hasNext()) {
        if (mutableiterator_java3.next() < 128)
            mutableiterator_java3.setValue(128);
    }
    while (mutableiterator_java3.hasPrevious())
        qDebug() << mutableiterator_java3.previous();
    qDebug() << "";

   // 值都乘2
    QMutableListIterator<int> mutableiterator_java4(mutableiterator1);
    while (mutableiterator_java4.hasNext())
        mutableiterator_java4.next() *= 2;

    while (mutableiterator_java4.hasPrevious())
        qDebug() << mutableiterator_java4.previous();
    qDebug() << "";

    QMap<QString, QString> mapiterator;
    mapiterator.insert("Paris", "France");
    mapiterator.insert("Guatemala City", "Guatemala");
    mapiterator.insert("Mexico City", "Mexico");
    mapiterator.insert("Moscow", "Russia");

    // 删除有City
    QMutableMapIterator<QString, QString> mutableiterator_java5(mapiterator);
    while (mutableiterator_java5.hasNext()) {
        if (mutableiterator_java5.next().key().endsWith("City"))
            mutableiterator_java5.remove();
    }
    while (mutableiterator_java5.hasPrevious())
        qDebug() << mutableiterator_java5.peekPrevious().key() << ':' << mutableiterator_java5.previous().value();
    qDebug() << "";

}

/**************************************************************
 *
 *                      stl迭代器
 *
 * ************************************************************/
void Widget::test_iteratorStl()
{
    qDebug() << "################stl迭代器################";

    //遍历,转换成小写
    QList<QString> listiteratorstl1;
    listiteratorstl1 << "A" << "B" << "C" << "D";
    QList<QString>::iterator listindex1;
    for (listindex1 = listiteratorstl1.begin(); listindex1 != listiteratorstl1.end(); ++listindex1)
    {
        *listindex1 = (*listindex1).toLower();
        qDebug() << *listindex1;
    }
    qDebug() << "";

    //反向迭代器遍历,转换成小写
    QList<QString> listiteratorstl2;
    listiteratorstl2 << "A" << "B" << "C" << "D";
    QList<QString>::reverse_iterator listindex2;
    for (listindex2 = listiteratorstl2.rbegin(); listindex2 != listiteratorstl2.rend(); ++listindex2){
        *listindex2 = listindex2->toLower();
        qDebug() << *listindex2;
    }
    qDebug() << "";

    //只读迭代器的遍历
    QList<QString>::const_iterator listindex3;
    for (listindex3 = listiteratorstl2.constBegin(); listindex3 != listiteratorstl2.constEnd(); ++listindex3)
        qDebug() << *listindex3;
    qDebug() << "";

    //关联容器，只读迭代器的遍历
    QMap<int, int> map;
    map.insert(12, 12);
    map.insert(13, 13);
    map.insert(2000, 2000);
    map.insert(5000, 5000);
    QMap<int, int>::const_iterator i;
    for (i = map.constBegin(); i != map.constEnd(); ++i)
        qDebug() << i.key() << ':' << i.value();

    qDebug() << "";
}

/**************************************************************
 *
 *                      foreach 关键字
 *
 * ************************************************************/
void Widget::test_foreach()
{
    qDebug() << "################foreach 关键字################";
    QList<QString> strlist = {"one", "two", "three"};
    QString str;

    // 遍历strlist
    foreach (str, strlist)
        qDebug() << str;
    qDebug() << "";

    foreach (const QString &str, strlist)
        qDebug() << str;
    qDebug() << "";

    foreach (const QString &str, strlist) {
        if (str.isEmpty())
            break;
        qDebug() << str;
    }
    qDebug() << "";

    QListIterator<QString> strindex(strlist);
    while (strindex.hasNext())
        qDebug() << strindex.next();
    qDebug() << "";

    //foreach访问（key，value）
    QMap<QString, int> strmap;
    strmap["one"] = 1;
    strmap["three"] = 3;
    strmap["seven"] = 7;
    strmap.insert("twelve", 12);
    foreach (const QString &str, strmap.keys())
        qDebug() << str << ':' << strmap.value(str);
    qDebug() <<"";

    //foreach访问（key，value）
    QMultiMap<QString, int> strmultimap;
    strmultimap.insert("twelve", 12);
    strmultimap.insert("plenty", 12);
    strmultimap.insert("plenty", 2000);
    strmultimap.insert("plenty", 5000);
    foreach (const QString &str, strmultimap.uniqueKeys()) {
        foreach (int i, strmultimap.values(str))
            qDebug() << str << ':' << i;
    }
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
