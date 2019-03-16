#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include <QImage>
#include <QByteArray>
#include <QVector2D>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_bLoad_clicked();

    void on_bGenerate_clicked();

private:

    Ui::Dialog *ui;

    float mE=0;
    QByteArray mGCode;

    QImage mImage;

    float mZ = 0.0f;

    QVector2D mCenter;

    QString mDefaultPath;

    void heating();
    void g21();
    void g28();
    void g90();
    void g92(const float e=0.0f);

    void g(const QString &str);
    void g0(const QVector3D v);
    void g1(const QVector3D v, const float e);

    QString toStr(const float val);
    QVector2D calcPos(const float angle, const float radius);

    bool checkImage();
    void generateGCODE(const QImage &image, const float radius, const float amplitude);
};

#endif // DIALOG_H
