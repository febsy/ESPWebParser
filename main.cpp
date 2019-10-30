#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QChar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

void prepareConfigString(QString &cfg)
{
    cfg.replace("\\","/");
    if (cfg.front() == ".")
    {
        cfg = QCoreApplication::applicationDirPath()+cfg.remove(0,1);
    }
    if (cfg.back() == "/")
    {
        cfg.chop(1);
    }
}

bool parseFile(const QString &filepath,QStringList &outputstring)
{
    QFile inputfile(filepath);
    if(inputfile.open(QFile::ReadWrite))
    {
        QByteArray input = inputfile.readAll();
        inputfile.close();
        outputstring.clear();
        for (int i = 0; i < input.length();i++)
        {
            char character = input.at(i);
            if ((character == '\n') || (character == '\r'))
            {
                continue;
            }
            QString charnr = QString::number(character,16);
            if (charnr.length() > 2)
            {
                int len = charnr.length();
                charnr = QString(charnr.at(len-2)) + QString(charnr.at(len-1));
                printf("INFO special character detected and recalculated\n");
                printf("please verify that this was done correctly\n");
                QStringList splittedpath = filepath.split("/");
                printf(("File: "+splittedpath.last()+"\n").toStdString().c_str());
                printf(("Position: "+QByteArray::number(i)+"\n").toStdString().c_str());
            }
            outputstring << "0x"+charnr;
        }
        return true;
    }
    return false;
}

bool createDefinition(const QString &filepath,QString filename,QString &outputstring)
{
    QStringList charlist;
    if(!parseFile(filepath,charlist))
    {
        return false;
    }
    outputstring += "const uint16_t "+filename.replace(".","_").toUpper()+"_SIZE = "+QString::number(charlist.length(),10)+";\n";
    outputstring += "const uint8_t "+filename.replace(".","_").toUpper()+"[] PROGMEM = {";
    for (int i=0;i<charlist.length();i++)
    {
        outputstring += charlist[i];
        if (i != (charlist.length()-1))
        {
            outputstring += ",";
        }
    }
    outputstring +="};\n";
    return true;
}

int main(int argc, char *argv[])
{
    printf("initializing...\n");
    QCoreApplication a(argc, argv);
    QFile file(QCoreApplication::applicationDirPath()+"/espwebparsercfg.json");
    if (!file.exists())
    {
        printf("ERROR could not find configuration file\n");
        printf("please put the 'espwebparsercfg.json file into the application directory\n");
        exit(1);
    }
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray cfgdata = file.readAll();
    file.close();
    QJsonDocument cfgdoc = QJsonDocument::fromJson(cfgdata);
    QJsonObject config = cfgdoc.object();
    QJsonValue inputdir = config.value("inputfolder");
    QJsonValue outputdir = config.value("outputfolder");
    if (inputdir == QJsonValue::Undefined)
    {
        printf("ERROR could not find key 'inputfolder' in config file\n");
        exit(2);
    }
    if (outputdir == QJsonValue::Undefined)
    {
        printf("ERROR could not find key 'outputfolder' in config file\n");
        exit(2);
    }
    QString inputdir_str = inputdir.toString();
    QString outputdir_str = outputdir.toString();
    if (inputdir_str.isNull())
    {
        printf("ERROR value for 'inputfolder' is not a string\n");
        exit(3);
    }
    if (outputdir_str.isNull())
    {
        printf("ERROR value for 'outputfolder' is not a string\n");
        exit(3);
    }
    prepareConfigString(inputdir_str);
    prepareConfigString(outputdir_str);

    QDir workingdir(inputdir_str);
    workingdir.setNameFilters(QStringList() <<"*.js"<<"*.html"<<"*.css");
    QStringList websitefiles = workingdir.entryList();
    QString headercontent = "";
    printf(("found "+QByteArray::number(websitefiles.length())+ " files\n").toStdString().c_str());
    int errorcount = 0;
    for (int i = 0;i < websitefiles.length();i++)
    {
        if(!createDefinition(inputdir_str+"/"+websitefiles[i],websitefiles[i],headercontent))
        {
            printf(("ERROR at file: "+websitefiles[i]+"\n").toUtf8().toStdString().c_str());
            errorcount++;
        }
        else
        {
            printf((websitefiles[i]+" done!\n").toUtf8().toStdString().c_str());
        }
    }
    printf("writing include header ...\n");
    QFile inc_header(outputdir_str+"/website.h");
    if (inc_header.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
    {
        inc_header.write(headercontent.toUtf8());
    }
    else
    {
        printf("ERROR could not create outputfile\n");
        exit(4);
    }
    inc_header.close();
    if (errorcount > 0)
    {
        printf(("ERROR finnished with "+QByteArray::number(errorcount)+" errors!\n").toStdString().c_str());
        exit(5);
    }
    printf("OK finnished without errors\n");
    exit(0);
}
