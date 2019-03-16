#include "dialog.h"
#include "ui_dialog.h"

#include <cmath>

#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QVector2D>
#include <QVector3D>
#include <QDateTime>
#include <QMessageBox>

#define qd qDebug()

float d2r(float deg)
{
    return deg * 0.0174533f;
}

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    mDefaultPath("D:/lithophane")
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

bool Dialog::checkImage()
{
    if(mImage.width() == 1200 && mImage.height() == 400) {
        ui->lImage->setPixmap(QPixmap::fromImage(mImage));
        return true;
    }

    return false;
}

void Dialog::on_bLoad_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), mDefaultPath, tr("Images (*.png *.jpg)"));
    int endPos = fileName.lastIndexOf("/");
    qd << endPos;
    if(endPos != -1) {
        mDefaultPath = fileName.mid(0, endPos);
    }
    mImage.load(fileName);
    if(checkImage())
    {
        QMessageBox::information(this, "Success", "Image loaded! Press Generate GCODE!");
    } else {
        QMessageBox::warning(this, "Error", "The image size must be 1200x400 pixels!");
    }
}

void Dialog::g(const QString &str)
{
    if(!str.endsWith("\r\n"))
    {
        mGCode.append(str);
        mGCode.append("\r\n");
    }
}

void Dialog::g21()
{
    mGCode.append( "G21\r\n" );
}

void Dialog::g28()
{
    mGCode.append( "G28\r\n" );
}

void Dialog::g90()
{
    mGCode.append( "G90\r\n" );
}

void Dialog::g92(const float e)
{
    mE = e;
    mGCode.append( "G92 E"+toStr(mE) + "\r\n" );
}

void Dialog::heating()
{
    mGCode.append("M104 S220\r\n");
    mGCode.append("M190 S60\r\n");
    mGCode.append("M109 S220\r\n" );
}

void Dialog::g0(const QVector3D v)
{
    QVector3D vec(v);
    vec += mCenter;

    mGCode.append( "G0 X" + toStr(vec.x()) + " Y" + toStr(vec.y()) + " F1000");
    mGCode.append( "\r\n" );
}

void Dialog::g1(const QVector3D v, const float e)
{
    QVector3D vec(v);
    vec += mCenter;

    mE += e;
    mGCode.append( "G1 X" + toStr(vec.x()) + " Y" + toStr(vec.y()) + " Z" + toStr(vec.z()) + " E" + toStr(mE) );
    mGCode.append( "\r\n" );
}

QString Dialog::toStr(const float val)
{
    return QString::number(static_cast<double>(val), 'f', 3);
}

QVector2D Dialog::calcPos(const float angle, const float radius)
{
    return QVector2D(sin(d2r(angle))*radius, cos(d2r(angle))*radius);
}

void Dialog::generateGCODE(const QImage& image, const float radius, const float amplitude)
{
    const float filaNozzleLHRatio = 20.0f;
    const float angleStep = 360.0f / image.width();
    const float LH = static_cast<float>(ui->sbLayerHeight->value());

    QVector3D prev;
    bool valid = false;
    bool inner = true;

    for(int h=0; h<image.height(); h++)
    {
        ui->pbTotal->setValue(h);
        QApplication::processEvents();

        uchar* scan = const_cast<uchar*>(image.scanLine(h));

        float depth = 0.0f;
        for(int jj=0; jj<image.width(); jj++)
        {
            float angle = jj * angleStep;

            QVector2D pos;
            if(inner) {
                pos = calcPos(angle, radius);
            } else {
                pos = calcPos(angle, radius+depth);
            }

            inner = !inner;

            QRgb* rgbpixel = reinterpret_cast<QRgb*>(scan + jj*4);
            int gray = qGray(*rgbpixel);
            depth = amplitude - ((gray / 255.0f) * amplitude);

            mZ += LH / (360.0f / angleStep);

            if(!valid) {
                valid = true;
                prev = QVector3D(pos, mZ);
                g0(prev);
                continue;
            } else {
                QVector3D temp(pos, mZ);
                float len = (temp - prev).length();
                g1(temp, len / filaNozzleLHRatio);
                prev = temp;

                if(inner) {
                    pos = calcPos(angle, radius);
                } else {
                    pos = calcPos(angle, radius+depth);
                }

                temp = QVector3D(pos, mZ);
                len = (temp - prev).length();
                g1(temp, len / filaNozzleLHRatio);
                prev = temp;
            }
        }
    }
}

void Dialog::on_bGenerate_clicked()
{
    if(!checkImage()) {
        QMessageBox::warning(this, "Error", "No valid image loaded!");
        return;
    }

    mCenter = QVector2D(ui->sbBedWidth->value()/2, ui->sbBedDepth->value()/2);

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), mDefaultPath + "/untitled.gcode", tr("GCODE (*.gco *.gcode)"));

    if(fileName.size() == 0) {
        QMessageBox::warning(this, "Error", "No proper GCODE file specified!");
        return;
    }

    ui->pbTotal->setMaximum(mImage.height());

    mGCode.clear();

    const float LH = static_cast<float>(ui->sbLayerHeight->value());

    mZ = LH;

    g28();
    heating();
    g21();
    g90();
    g92();
    g("G0 Z" + toStr(mZ));

    generateGCODE(QImage(":/header_footer.jpg"), 72, 7);
    generateGCODE(mImage, 76, 1.5);
    generateGCODE(QImage(":/header_footer.jpg"), 76, 3);

    g("G0 X0 Y0");

    QFile f(fileName);
    if(f.open(QFile::WriteOnly)) {
        f.write(mGCode);
        f.close();
    }

    ui->pbTotal->setValue(ui->pbTotal->maximum());

    QMessageBox::information(this, "Success", "GCODE saved!");
}
