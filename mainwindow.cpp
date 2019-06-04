/*-------------------------------------------------
#
# neural-style GUI with QT by AbsurdePhoton
#
# v1 2018-01-08
#
#-------------------------------------------------*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QThread"
#include <QWaitCondition>
#include <QMutex>

void threadSleep(unsigned long ms)
{
    QMutex mutex;
    mutex.lock();
    QWaitCondition waitCond;
    waitCond.wait(&mutex, ms);
    mutex.unlock();
}

void MainWindow::Abort()
{
    ui->result->moveCursor (QTextCursor::End);
    ui->result->insertHtml("<br><br><span style=\"color:#ff0000;\">***** Aborted ****</span><br><br>");
    ui->result->moveCursor (QTextCursor::End);
    QString path = "";
    QPixmap p = path;
    ui->imageResult->setPixmap(p);
    ui->progress->setValue(ui->progress->minimum());
    if (running) {
        disconnect(process, 0, 0, 0);
        process->terminate();
        process->waitForFinished();
        delete process;
    }
}

void MainWindow::Run()
{
    QString text;
    text = "th " + QCoreApplication::applicationDirPath() + "/neural_style.lua";
    if (ui->fileSource->text() != "") text += " -content_image " + ui->fileSource->text(); else {QMessageBox::information(this, "Required !", "Source file required");return;}
    if (ui->fileStyle->text() != "") text += " -style_image " + ui->fileStyle->text(); else {QMessageBox::information(this, "Required !", "At least one style file required");return;}
    if (ui->fileStyle2->text() != "") text += "," + ui->fileStyle2->text();
    if (ui->output->text() != "") text += " -output_image ./output/" + ui->output->text() + ".png"; else {QMessageBox::information(this, "Required !", "Output directory required");return;}
    text += " -num_iterations " + ui->iterations->text();
    ui->progress->setRange(0, ui->iterations->value());
    text += " -print_iter " + ui->print->text();
    text += " -save_iter " + ui->checkpoint->text();
    text += " -init " + ui->init->currentText();
    text += " -image_size " + ui->width->text();
    text += " -style_scale " + QString::number((double)ui->styleScale->value() / 100);
    if (ui->network->text() != "") text += " -model_file " + ui->network->text(); else {QMessageBox::information(this, "Required !", "The network file is required");return;}
    text += " -learning_rate " + QString::number(ui->learningRate->value());
    text += " -content_weight " + QString::number(ui->contentWeight->value());
    text += " -style_weight " + QString::number(ui->styleWeight->value());
    text += " -style_blend_weights " + QString::number(ui->styleBlendWeight->value()/(float)100);
    if (ui->fileStyle2->text() != "") text += "," + QString::number(ui->styleBlendWeight2->value()/(float)100);
    text += " -tv_weight 1e-" + QString::number(ui->tvWeight->value());
    if (ui->contentLayers->text() != "") text += " -content_layers " + ui->contentLayers->text();
    if (ui->styleLayers->text() != "") text += " -style_layers " + ui->styleLayers->text();
    text += " -optimizer " + ui->optimizer->currentText();
    text += " -pooling " + ui->pooling->currentText();
    text += " -gpu " + QString::number(ui->gpu->value());
    text += " -backend " + ui->backend->currentText();
    if ((ui->cudnn->isChecked()) && (ui->backend->currentText() == "cudnn")) text += " -cudnn_autotune";
    if (ui->normalizeGradients->isChecked()) text += " -normalize_gradients";
    if (ui->originaColors->isChecked()) text += " -original_colors 1"; else text += " -original_colors 0";

    QMessageBox msgBox;
    msgBox.setWindowTitle("This command line will be executed");
    msgBox.setText(text);
    msgBox.setStandardButtons(QMessageBox::Apply | QMessageBox::Save | QMessageBox::Close);
    msgBox.setDefaultButton(QMessageBox::Apply);
    int ret = msgBox.exec();

    if (ret==QMessageBox::Apply || ret==QMessageBox::Save) {
        // Try and open a file for output
        QString outputFilename = "./command.sh";
        QFile outputFile(outputFilename);
        outputFile.open(QIODevice::WriteOnly);

        // Check it opened OK
        if(!outputFile.isOpen()){
            QMessageBox::information(this, "Writing file", "Error");
            return;
        }

        // Set file permissions for execution
        outputFile.setPermissions(QFile::ReadUser | QFile::WriteUser | QFile::ExeUser | QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup | QFile::ReadOther | QFile::WriteOther | QFile::ExeOther);

        // Point a QTextStream object at the file
        QTextStream outStream(&outputFile);

        // Write the lines to the file
        outStream << "#!/bin/bash\nmkdir ./output\n" << text << "\n";
        //outStream << "#!/bin/bash" << "ls -al /media/Photo/Photos" << "\n";

        //Close the file
        outputFile.close();
    }

    if (ret==QMessageBox::Apply) {
        // Run script in a new process
        process = new QProcess();
        connect(process, SIGNAL(readyReadStandardError()), this, SLOT(ErrorOutput()));
        connect(process, SIGNAL(started()), this, SLOT(Started()));
        connect(process, SIGNAL(readyReadStandardOutput()),this,SLOT(StandardOutput()));
        connect(process, SIGNAL(finished(int)), this, SLOT(Finished()));
        process->start("./command.sh");
        process->waitForStarted();
        running = true;
    }
}

bool fileExists(QString path) {
    QFileInfo check_file(path);
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile()) {
        return true;
    } else {
        return false;
    }
}

void MainWindow::ErrorOutput()
{
    QString result = process->readAllStandardError();
    ui->result->moveCursor (QTextCursor::End);
    ui->result->insertHtml("<br><span style=\"color:#ff0000;\">" + result + "</span>");
    ui->result->moveCursor (QTextCursor::End);
}

void MainWindow::Started()
{
    ui->result->setHtml("<span style=\"color:#00ffff;\">***** Started ****</span><br><br>");
    QString path = "";
    QPixmap p = path;
    ui->imageResult->setPixmap(p);
    ui->progress->setValue(ui->progress->minimum());
}

void MainWindow::StandardOutput()
{
    QString result = process->readAllStandardOutput();
    ui->result->moveCursor (QTextCursor::End);
    ui->result->insertHtml("<br><span style=\"color:#00ff00;\">" + result + "</span>");
    ui->result->moveCursor (QTextCursor::End);
    // If it is an iteration
    if (result.mid(0,9) == "Iteration") {
        // Set the iteration number to the progress bar
        int espace = result.indexOf("Iteration ",0) + 10;
        int virgule = result.indexOf(" ",espace);
        QString nombre = result.mid(espace,virgule-espace);
        bool ok;
        int valeur = nombre.toInt(&ok,10);
        if (ok) {
            ui->progress->setValue(valeur);
            // Test if the iteration results in intermediate image output
            if (valeur % ui->checkpoint->value() == 0) {
                QString path = "";
                // Intermediate image
                if (valeur < ui->iterations->value()) path = "./output/" + ui->output->text() + "_" + QString::number(valeur) + ".png"; else path = "./output/" + ui->output->text() + ".png";
                ui->result->moveCursor (QTextCursor::End);
                ui->result->insertHtml("<br><span style=\"color:#ff00ff;\">Intermediate output : " + path + "</span>");
                ui->result->moveCursor (QTextCursor::End);
                // Load image to display
                int w = ui->imageResult->width();
                int h = ui->imageResult->height();
                ui->result->moveCursor (QTextCursor::End);
                ui->result->insertHtml("<br><span style=\"color:#ffffff;\">Wait for image : " + path);
                ui->result->moveCursor (QTextCursor::End);
                while (!fileExists(path)) {
                    ui->result->insertHtml(".");
                }
                ui->result->insertHtml("</span><br>");
                ui->result->moveCursor (QTextCursor::End);
                threadSleep(1000);
                QPixmap p = path;
                // Assigne au label l'image à la bonne taille
                ui->imageResult->setPixmap(p.scaled(w,h,Qt::KeepAspectRatio));
            }
        }
    }
}

void MainWindow::Finished()
{
    ui->result->moveCursor(QTextCursor::End);
    ui->result->insertHtml("<br><br><span style=\"color:#00ffff;\">***** Finished ! *****</span>\n");
    ui->result->moveCursor(QTextCursor::End);
    ui->progress->setValue(ui->progress->maximum());
    delete process;
    running = false;
}

void MainWindow::SelectImageSource() //Open source image
{
    // Dialogue pour ouvrir l'image
    QString fileName = QFileDialog::getOpenFileName(this,"Open source image", ".", tr("Image Files (*.png *.PNG *.jpg *.JPG *.jpeg *.JPEG *.tif *.TIF *.tiff *.TIFF)"));
    // Met à jour la label avec le nom de fichier choisi
    ui->fileSource->setText(fileName);
    // Dimension du label d'affichage
    int w = ui->imageSource->width();
    int h = ui->imageSource->height();
    // Met l'image dans une variable
    QPixmap p = fileName;
    // Assigne au label l'image à la bonne taille
    ui->imageSource->setPixmap(p.scaled(w,h,Qt::KeepAspectRatio));
}

void MainWindow::SelectImageStyle() //Open style image
{
    // Dialogue pour ouvrir l'image
    QString fileName = QFileDialog::getOpenFileName(this,"Open source image", "../Styles", tr("Image Files (*.png *.PNG *.jpg *.JPG *.jpeg *.JPEG *.tif *.TIF *.tiff *.TIFF)"));
    // Met à jour la label avec le nom de fichier choisi
    ui->fileStyle->setText(fileName);
    // Dimension du label d'affichage
    int w = ui->imageStyle->width();
    int h = ui->imageStyle->height();
    // Met l'image dans une variable
    QPixmap p = fileName;
    int maxSize = p.width();
    if (p.height() > maxSize) maxSize = p.height();
    // Assigne au label l'image à la bonne taille
    ui->imageStyle->setPixmap(p.scaled(w,h,Qt::KeepAspectRatio));
}

void MainWindow::SelectImageStyle2() //Open second style image
{
    // Dialogue pour ouvrir l'image
    QString fileName = QFileDialog::getOpenFileName(this,"Open source image", "../Styles", tr("Image Files (*.png *.jpg *.tif)"));
    // Met à jour la label avec le nom de fichier choisi
    ui->fileStyle2->setText(fileName);
    if (fileName != "") {
        // Dimension du label d'affichage
        int w = ui->imageStyle2->width();
        int h = ui->imageStyle2->height();
        // Met l'image dans une variable
        QPixmap p = fileName;
        // Assigne au label l'image à la bonne taille
        ui->imageStyle2->setPixmap(p.scaled(w,h,Qt::KeepAspectRatio));
        // Intitialise les valeurs pour la nouvelle image
        ui->styleBlendWeight2->setVisible(true);
        ui->imageStyle2->setVisible(true);
        ui->styleBlendWeight->setValue(50);
        ui->styleBlendWeight2->setValue(50);}
    else {
        // Intitialise les valeurs en supprimant l'image
        ui->styleBlendWeight2->setVisible(false);
        ui->imageStyle2->setVisible(false);
        ui->styleBlendWeight->setValue(100);
        ui->styleBlendWeight2->setValue(0);
    }
}

void MainWindow::SelectNetwork() //Open network caffe file
{
    // Dialogue pour ouvrir l'image
    QString fileName = QFileDialog::getOpenFileName(this,"Select network to use...", "./models/", tr("caffe models (*.caffemodel)"));
    // Met à jour la label avec le nom de fichier choisi
    ui->network->setText(fileName);
}

void MainWindow::ChangeStyleBlendWeight()
{
    if (ui->fileStyle2->text() == "") {ui->styleBlendWeight->setValue(100);ui->styleBlendWeight2->setValue(0);return;}
    if (ui->styleBlendWeight->hasFocus()) {ui->styleBlendWeight2->setValue(100-ui->styleBlendWeight->value());return;}
    if (ui->styleBlendWeight2->hasFocus()) ui->styleBlendWeight->setValue(100-ui->styleBlendWeight2->value());
}

void MainWindow::ChangeBackend()
{
    if (ui->backend->currentText() == "cudnn") ui->cudnn->setVisible(true); else ui->cudnn->setVisible(false);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->run, SIGNAL(clicked()), this, SLOT(Run()));
    connect(ui->abort, SIGNAL(clicked()), this, SLOT(Abort()));
    connect(ui->buttonSource, SIGNAL(clicked()), this, SLOT(SelectImageSource()));
    connect(ui->buttonStyle, SIGNAL(clicked()), this, SLOT(SelectImageStyle()));
    connect(ui->buttonStyle2, SIGNAL(clicked()), this, SLOT(SelectImageStyle2()));
    connect(ui->buttonNetwork, SIGNAL(clicked()), this, SLOT(SelectNetwork()));
    connect(ui->styleBlendWeight, SIGNAL(valueChanged(int)), this, SLOT(ChangeStyleBlendWeight()));
    connect(ui->styleBlendWeight2, SIGNAL(valueChanged(int)), this, SLOT(ChangeStyleBlendWeight()));
    connect(ui->backend, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangeBackend()));

    ui->optimizer->addItem(tr("lbfgs"));
    ui->optimizer->addItem(tr("adam"));
    ui->backend->addItem(tr("cudnn"));
    ui->backend->addItem(tr("clnn"));
    ui->backend->addItem(tr("nn"));
    ui->pooling->addItem(tr("max"));
    ui->pooling->addItem(tr("avg"));
    ui->init->addItem(tr("image"));
    ui->init->addItem(tr("random"));
    ui->progress->setRange(0, 100);
    ui->progress->setValue(0);
    ui->imageStyle2->setVisible(false);
    ui->styleBlendWeight2->setVisible(false);
    running = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}
