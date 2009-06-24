#include "surfaceproperties.h"
#include "shadereditor.h"
#include "gfx.h"
#include "mesh.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QInputDialog>
#include <QPaintEngine>

SurfaceProperties *SurfaceProperties::pInstance = NULL;

SurfaceProperties *SurfaceProperties::instance()
{
    if (!pInstance)
        pInstance = new SurfaceProperties;
    return pInstance;
}

SurfaceProperties::SurfaceProperties()
{
    updating = true;
    red = new QSpinBox;
    green = new QSpinBox;
    blue = new QSpinBox;
    alpha = new QSpinBox;
    redR = new QSpinBox;
    greenR = new QSpinBox;
    blueR = new QSpinBox;
    alphaR = new QSpinBox;

    red->setMinimum(0);
    red->setMaximum(255);
    green->setMinimum(0);
    green->setMaximum(255);
    blue->setMinimum(0);
    blue->setMaximum(255);
    alpha->setMinimum(0);
    alpha->setMaximum(255);
    redR->setMinimum(0);
    redR->setMaximum(255);
    greenR->setMinimum(0);
    greenR->setMaximum(255);
    blueR->setMinimum(0);
    blueR->setMaximum(255);
    alphaR->setMinimum(0);
    alphaR->setMaximum(255);

    flagBlended = new QCheckBox(tr("Blended"));
    flagGLSL = new QCheckBox(tr("GLSL"));
    flagGouraud = new QCheckBox(tr("Gouraud"));
    flagLighted = new QCheckBox(tr("Lighted"));
    flagPlayerColor = new QCheckBox(tr("Player Color"));
    flagReflec = new QCheckBox(tr("Reflective"));
    flagTextured = new QCheckBox(tr("Textured"));
    flagRootTexture = new QCheckBox(tr("Share textures"));

    QFormLayout *colorLayout = new QFormLayout;
    colorLayout->addRow( new QLabel(tr("object color:")));
    colorLayout->addRow( new QLabel(tr("red")), red);
    colorLayout->addRow( new QLabel(tr("green")), green);
    colorLayout->addRow( new QLabel(tr("blue")), blue);
    colorLayout->addRow( new QLabel(tr("alpha")), alpha);

    QFormLayout *colorRLayout = new QFormLayout;
    colorRLayout->addRow( new QLabel(tr("reflection color:")));
    colorRLayout->addRow( new QLabel(tr("red")), redR);
    colorRLayout->addRow( new QLabel(tr("green")), greenR);
    colorRLayout->addRow( new QLabel(tr("blue")), blueR);
    colorRLayout->addRow( new QLabel(tr("alpha")), alphaR);

    QHBoxLayout *colorsLayout = new QHBoxLayout;
    colorsLayout->addLayout(colorLayout);
    colorsLayout->addSpacing(10);
    colorsLayout->addLayout(colorRLayout);

    QGridLayout *flagLayout = new QGridLayout;
    flagLayout->addWidget(flagBlended, 0, 0);
    flagLayout->addWidget(flagGLSL, 1, 0);
    flagLayout->addWidget(flagGouraud, 2, 0);
    flagLayout->addWidget(flagLighted, 3, 0);
    flagLayout->addWidget(flagPlayerColor, 0, 1);
    flagLayout->addWidget(flagReflec, 1, 1);
    flagLayout->addWidget(flagTextured, 2, 1);
    flagLayout->addWidget(flagRootTexture, 3, 1);
    QPushButton *bShader = new QPushButton(tr("Shader"));
    flagLayout->addWidget(bShader, 4, 0);

    QVBoxLayout *finalLayout = new QVBoxLayout;
    finalLayout->addLayout(colorsLayout);
    finalLayout->addLayout(flagLayout);

    finalLayout->addWidget( new QLabel(tr("Textures:")) );

    QGridLayout *textureLayout = new QGridLayout;

    imageListView = new ImageListView;
    finalLayout->addWidget(imageListView);

    QPushButton *bLoad = new QPushButton(tr("&Load"));
    QPushButton *bSave = new QPushButton(tr("&Save"));
    QPushButton *bNew = new QPushButton(tr("&New"));
    QPushButton *bDelete = new QPushButton(tr("&Delete"));
    QPushButton *bLeft = new QPushButton(tr("&<"));
    QPushButton *bRight = new QPushButton(tr("&>"));

    QPushButton *bBasic = new QPushButton(tr("&basic UV"));
    QPushButton *bSpherical = new QPushButton(tr("&spherical UV"));
    QPushButton *bAuto = new QPushButton(tr("&auto UV"));
    QPushButton *bMerge = new QPushButton(tr("&merge vertices"));
    QPushButton *bAmbientOcclusion = new QPushButton(tr("&ambient occlusion"));
    QPushButton *bFlip = new QPushButton(tr("&flip"));
    textureLayout->addWidget(bLoad, 0, 0);
    textureLayout->addWidget(bSave, 0, 1);
    textureLayout->addWidget(bNew, 1, 0);
    textureLayout->addWidget(bDelete, 1, 1);
    textureLayout->addWidget(bLeft, 2, 0);
    textureLayout->addWidget(bRight, 2, 1);
    textureLayout->addWidget(bBasic, 3, 0);
    textureLayout->addWidget(bSpherical, 3, 1);
    textureLayout->addWidget(bAuto, 4, 0);
    textureLayout->addWidget(bMerge, 4, 1);
    textureLayout->addWidget(bAmbientOcclusion, 5, 0);
    textureLayout->addWidget(bFlip, 5, 1);

    finalLayout->addLayout(textureLayout);

    setLayout(finalLayout);

    // GUI connections

    connect(red, SIGNAL(valueChanged(int)), this, SLOT(readData()));
    connect(green, SIGNAL(valueChanged(int)), this, SLOT(readData()));
    connect(blue, SIGNAL(valueChanged(int)), this, SLOT(readData()));
    connect(alpha, SIGNAL(valueChanged(int)), this, SLOT(readData()));
    connect(redR, SIGNAL(valueChanged(int)), this, SLOT(readData()));
    connect(greenR, SIGNAL(valueChanged(int)), this, SLOT(readData()));
    connect(blueR, SIGNAL(valueChanged(int)), this, SLOT(readData()));
    connect(alphaR, SIGNAL(valueChanged(int)), this, SLOT(readData()));

    connect(flagBlended, SIGNAL(stateChanged(int)), this, SLOT(readData()));
    connect(flagGLSL, SIGNAL(stateChanged(int)), this, SLOT(readData()));
    connect(flagGouraud, SIGNAL(stateChanged(int)), this, SLOT(readData()));
    connect(flagLighted, SIGNAL(stateChanged(int)), this, SLOT(readData()));
    connect(flagPlayerColor, SIGNAL(stateChanged(int)), this, SLOT(readData()));
    connect(flagReflec, SIGNAL(stateChanged(int)), this, SLOT(readData()));
    connect(flagTextured, SIGNAL(stateChanged(int)), this, SLOT(readData()));
    connect(flagRootTexture, SIGNAL(stateChanged(int)), this, SLOT(readData()));

    connect(bShader, SIGNAL(clicked()), ShaderEditor::instance(), SLOT(show()));

    connect(bLoad, SIGNAL(clicked()), this, SLOT(loadTexture()));
    connect(bSave, SIGNAL(clicked()), this, SLOT(saveTexture()));
    connect(bNew, SIGNAL(clicked()), this, SLOT(newTexture()));
    connect(bDelete, SIGNAL(clicked()), this, SLOT(deleteTexture()));
    connect(bLeft, SIGNAL(clicked()), this, SLOT(moveTextureLeft()));
    connect(bRight, SIGNAL(clicked()), this, SLOT(moveTextureRight()));

    connect(bBasic, SIGNAL(clicked()), this, SLOT(basicUV()));
    connect(bSpherical, SIGNAL(clicked()), this, SLOT(sphericalUV()));
    connect(bAuto, SIGNAL(clicked()), this, SLOT(autoUV()));
    connect(bMerge, SIGNAL(clicked()), this, SLOT(mergeVertices()));
    connect(bAmbientOcclusion, SIGNAL(clicked()), this, SLOT(computeAmbientOcclusion()));
    connect(bFlip, SIGNAL(clicked()), this, SLOT(flipTexture()));

    updateWindowTitle();

    updating = false;
}

void SurfaceProperties::updateWindowTitle()
{
    int ID = Gfx::instance()->getSelectionID();
    if (ID >= 0)
        setWindowTitle(tr("Surface properties") + " - " + Mesh::instance()->getMesh(ID)->getName());
    else
        setWindowTitle(tr("Surface properties"));
}

void SurfaceProperties::refreshGUI()
{
    updateWindowTitle();
    int ID = Gfx::instance()->getSelectionID();
    updating = true;
    if (ID >= 0)
    {
        Mesh *mesh = Mesh::instance()->getMesh(ID);
        red->setValue((mesh->color >> 24));
        green->setValue((mesh->color >> 16) & 0xFF);
        blue->setValue((mesh->color >> 8) & 0xFF);
        alpha->setValue(mesh->color & 0xFF);
        redR->setValue((mesh->rColor >> 24) & 0xFF);
        greenR->setValue((mesh->rColor >> 16) & 0xFF);
        blueR->setValue((mesh->rColor >> 8) & 0xFF);
        alphaR->setValue(mesh->rColor & 0xFF);

        flagBlended->setChecked(mesh->flag & SURFACE_BLENDED);
        flagGLSL->setChecked(mesh->flag & SURFACE_GLSL);
        flagGouraud->setChecked(mesh->flag & SURFACE_GOURAUD);
        flagLighted->setChecked(mesh->flag & SURFACE_LIGHTED);
        flagPlayerColor->setChecked(mesh->flag & SURFACE_PLAYER_COLOR);
        flagReflec->setChecked(mesh->flag & SURFACE_REFLEC);
        flagTextured->setChecked(mesh->flag & SURFACE_TEXTURED);

        QList<QImage> imageList;
        mesh = getMeshWithTextures();
        for(int i = 0 ; i < mesh->tex.size() ; i++)
        {
            QImage image = Gfx::instance()->textureToImage( mesh->tex[i] );
            imageList.push_back( image.scaled(128, 128) );
        }
        imageListView->setImageList( imageList );
    }
    else
    {
        red->setValue(0);
        green->setValue(0);
        blue->setValue(0);
        alpha->setValue(0);
        redR->setValue(0);
        greenR->setValue(0);
        blueR->setValue(0);
        alphaR->setValue(0);

        flagBlended->setChecked(false);
        flagGLSL->setChecked(false);
        flagGouraud->setChecked(false);
        flagLighted->setChecked(false);
        flagPlayerColor->setChecked(false);
        flagReflec->setChecked(false);
        flagTextured->setChecked(false);
        imageListView->setImageList( QList<QImage>() );
    }
    updating = false;
}

void SurfaceProperties::readData()
{
    if (updating)
        return;
    int ID = Gfx::instance()->getSelectionID();
    if (ID >= 0)
    {
        Mesh *mesh = Mesh::instance()->getMesh(ID);
        mesh->color = 0;
        mesh->color |= red->value() << 24;
        mesh->color |= green->value() << 16;
        mesh->color |= blue->value() << 8;
        mesh->color |= alpha->value();
        mesh->rColor = 0;
        mesh->rColor |= redR->value() << 24;
        mesh->rColor |= greenR->value() << 16;
        mesh->rColor |= blueR->value() << 8;
        mesh->rColor |= alphaR->value();

        mesh->flag = SURFACE_ADVANCED;
        mesh->flag |= flagBlended->isChecked() ? SURFACE_BLENDED : 0;
        mesh->flag |= flagGLSL->isChecked() ? SURFACE_GLSL : 0;
        mesh->flag |= flagGouraud->isChecked() ? SURFACE_GOURAUD : 0;
        mesh->flag |= flagLighted->isChecked() ? SURFACE_LIGHTED : 0;
        mesh->flag |= flagPlayerColor->isChecked() ? SURFACE_PLAYER_COLOR : 0;
        mesh->flag |= flagReflec->isChecked() ? SURFACE_REFLEC : 0;
        mesh->flag |= flagTextured->isChecked() ? SURFACE_TEXTURED : 0;

        QList<Mesh*> lMesh = Mesh::instance()->getSubList();                             // SURFACE_ROOT_TEXTURE should have the same value across all sub meshes
        foreach(Mesh *cur, lMesh)
            cur->flag = (cur->flag & ~SURFACE_ROOT_TEXTURE) | (flagRootTexture->isChecked() ? SURFACE_ROOT_TEXTURE : 0);

        refreshGUI();
        emit surfaceChanged();
    }
}

Mesh *SurfaceProperties::getMeshWithTextures()
{
    if (Mesh::instance()->flag & SURFACE_ROOT_TEXTURE)
        return Mesh::instance();
    int ID = Gfx::instance()->getSelectionID();
    return Mesh::instance()->getMesh(ID);
}

void SurfaceProperties::newTexture()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    if (mesh->flag & SURFACE_ROOT_TEXTURE)
        mesh = Mesh::instance();
    int w = QInputDialog::getInt(this,tr("Texture width"), tr("Enter texture width:"), 128, 1, 1024);
    int h = QInputDialog::getInt(this,tr("Texture height"), tr("Enter texture height:"), 128, 1, 1024);

    Gfx::instance()->makeCurrent();
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(0xFFFFFFFF);       // Fill image with white
    if (!mesh->tcoord.isEmpty())    // If we have a set of UV coordinates, then draw them on the image
    {
        QPainter painter(&img);
        switch(mesh->type)
        {
        case MESH_TRIANGLES:
            for(int i = 0 ; i < mesh->index.size() ; i += 3)
            {
                QVector<QLineF> triangle;
                triangle.push_back( QLineF(mesh->tcoord[mesh->index[i] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i] * 2 + 1]) * h,
                                           mesh->tcoord[mesh->index[i + 1] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i + 1] * 2 + 1]) * h) );
                triangle.push_back( QLineF(mesh->tcoord[mesh->index[i] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i] * 2 + 1]) * h,
                                           mesh->tcoord[mesh->index[i + 2] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i + 2] * 2 + 1]) * h) );
                triangle.push_back( QLineF(mesh->tcoord[mesh->index[i + 2] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i + 2] * 2 + 1]) * h,
                                           mesh->tcoord[mesh->index[i + 1] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i + 1] * 2 + 1]) * h) );
                painter.drawLines(triangle);
            }
            break;
        case MESH_TRIANGLE_STRIP:
            for(int i = 0 ; i < mesh->index.size() - 1 ; i++)
            {
                QLineF line(mesh->tcoord[mesh->index[i] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i] * 2 + 1]) * h,
                            mesh->tcoord[mesh->index[i + 1] * 2] * w, (1.0f - mesh->tcoord[mesh->index[i + 1] * 2 + 1]) * h);
                painter.drawLine(line);
            }
            break;
        };
    }
    mesh->tex.push_back(Gfx::instance()->bindTexture(img));
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::deleteTexture()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    int textureID = imageListView->selectedIndex();
    if (textureID < 0 || textureID >= mesh->tex.size())
        return;
    Gfx::instance()->deleteTexture(mesh->tex[textureID]);
    mesh->tex.remove(textureID);
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::loadTexture()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    QString filename = QFileDialog::getOpenFileName(this,tr("Load texture"),QString(),tr("all files(*.*);;jpeg images(*.jpg);;png images(*.png)"));
    if (filename.isEmpty())
        return;
    mesh->tex.push_back(Gfx::instance()->loadTexture(filename));
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::flipTexture()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    int textureID = imageListView->selectedIndex();
    if (textureID < 0 || textureID >= mesh->tex.size())
        return;
    QImage tex = Gfx::instance()->textureToImage(mesh->tex[textureID]);//.mirrored(false, true);
    Gfx::instance()->deleteTexture(mesh->tex[textureID]);
    mesh->tex[textureID] = Gfx::instance()->bindTexture(tex);
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::saveTexture()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    int textureID = imageListView->selectedIndex();
    if (textureID < 0 || textureID >= mesh->tex.size())
        return;
    QString filename = QFileDialog::getSaveFileName(this,tr("Save texture as"),QString(),tr("all files(*.*);;jpeg images(*.jpg);;png images(*.png)"));
    if (filename.isEmpty())
        return;
    QImage img = Gfx::instance()->textureToImage( mesh->tex[textureID] );
    img.save(filename);
}

void SurfaceProperties::moveTextureLeft()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    int textureID = imageListView->selectedIndex();
    if (textureID < 1 || textureID >= mesh->tex.size())
        return;
    mesh->tex[textureID] ^= mesh->tex[textureID - 1];
    mesh->tex[textureID - 1] ^= mesh->tex[textureID];
    mesh->tex[textureID] ^= mesh->tex[textureID - 1];
    refreshGUI();
    emit surfaceChanged();
    imageListView->selectIndex(textureID - 1);
}

void SurfaceProperties::moveTextureRight()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    int textureID = imageListView->selectedIndex();
    if (textureID < 0 || textureID + 1>= mesh->tex.size())
        return;
    mesh->tex[textureID] ^= mesh->tex[textureID + 1];
    mesh->tex[textureID + 1] ^= mesh->tex[textureID];
    mesh->tex[textureID] ^= mesh->tex[textureID + 1];
    refreshGUI();
    emit surfaceChanged();
    imageListView->selectIndex(textureID + 1);
}

void SurfaceProperties::computeTexturePartition()
{
    QList<Mesh*> lMesh = Mesh::instance()->getSubList();
    int i = 0;
    int s = sqrt(lMesh.size());
    if (s * s < lMesh.size())
        ++s;
    foreach(Mesh *cur, lMesh)
    {
        for(int e = 0 ; e < cur->tcoord.size() ; e += 2)
        {
            cur->tcoord[e] = cur->tcoord[e] / s + ((float)(i % s)) / s;
            cur->tcoord[e+1] = cur->tcoord[e+1] / s + ((float)(i / s)) / s;
        }
        ++i;
    }
}

void SurfaceProperties::basicUV()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    if (mesh->flag & SURFACE_ROOT_TEXTURE)
    {
        foreach(Mesh *cur, Mesh::instance()->getSubList())
            cur->basicMapping();
        computeTexturePartition();
    }
    else
        mesh->basicMapping();
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::sphericalUV()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    if (mesh->flag & SURFACE_ROOT_TEXTURE)
    {
        foreach(Mesh *cur, Mesh::instance()->getSubList())
            cur->sphericalMapping();
        computeTexturePartition();
    }
    else
        mesh->sphericalMapping();
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::autoUV()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    if (mesh->flag & SURFACE_ROOT_TEXTURE)
    {
        foreach(Mesh *cur, Mesh::instance()->getSubList())
            cur->autoComputeUVcoordinates();
        computeTexturePartition();
    }
    else
        mesh->autoComputeUVcoordinates();
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::mergeVertices()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    mesh->mergeSimilarVertices();
    refreshGUI();
    emit surfaceChanged();
}

void SurfaceProperties::computeAmbientOcclusion()
{
    Mesh *mesh = getMeshWithTextures();
    if (mesh == NULL)
        return;
    int w = QInputDialog::getInt(this,tr("Texture width"), tr("Enter texture width:"), 128, 1, 1024);
    int h = QInputDialog::getInt(this,tr("Texture height"), tr("Enter texture height:"), w, 1, 1024);
    int precision = QInputDialog::getInt(this,tr("Precision"), tr("Enter required precision (1-1024)"), 10, 1, 1024);

    Gfx::instance()->makeCurrent();
    mesh->computeAmbientOcclusion(w, h, Mesh::instance(), precision);
    refreshGUI();
    emit surfaceChanged();
}
