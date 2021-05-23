#include "mainwindow.h"

#include "Gglobal.h"
#include "ModelView.h"

#include <iostream>

#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QMessageBox>
#include <QToolBar>
#include <QVBoxLayout>

#include <AIS_InteractiveObject.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <OpenGl_GraphicDriver.hxx>
#if !defined(_WIN32) && !defined(__WIN32__) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX))
#include <OSD_Environment.hxx>
#endif
#include <TCollection_AsciiString.hxx>
#include <TopExp_Explorer.hxx>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(720, 540);

    // 获取3D渲染的Context
    TCollection_ExtendedString a3Dname("Visu3D");

    // 这里选择空间坐标系的方向，其中V3D_XposYnegZpos代表X,Y,Z都有，但是Y选择负方向
    myV3dViewer = Viewer(a3Dname.ToExtString(), "", 1000.0, V3d_XposYnegZpos, Standard_True, Standard_True);

    // 这里配置光照信息，打光采用默认的光源，同时启动光照计算
    myV3dViewer->SetDefaultLights();
    myV3dViewer->SetLightOn();

    // 记录myV3dViewer的context信息
    myContext = new AIS_InteractiveContext(myV3dViewer);

    // 初始化一个内部界面布局
    QFrame *     vb     = new QFrame(this);
    QVBoxLayout *layout = new QVBoxLayout(vb);
    layout->setMargin(0);
    vb->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setCentralWidget(vb);

    // 初始化带有viewcube的myView
    myView = new ModelView(myContext, true, vb);
    layout->addWidget(myView);
    connect(myView, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));

    // 初始化View、RayTrace控制相关的Toolbar
    createDisplaymodeActions();
    createViewActions();
    createRaytraceActions();

    setStatusTip(tr("鼠标按键: 左键-旋转，Alt+左键-框选, 中键-平移，右键-缩放"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::dump()
{
    QString     filter = "Images Files (*.bmp *.ppm *.png *.jpg *.tiff *.tga *.gif *.exr)";
    QFileDialog fd(0);
    fd.setModal(true);
    fd.setNameFilter(filter);
    fd.setWindowTitle(QObject::tr("导出数据"));
    fd.setFileMode(QFileDialog::AnyFile);
    int ret = fd.exec();

    /* update the desktop after the dialog is closed */
    qApp->processEvents();

    QStringList fileNames;
    fileNames = fd.selectedFiles();

    QString file((ret == QDialog::Accepted && !fileNames.isEmpty()) ? fileNames[0] : nullptr);
    if (!file.isNull())
    {
        //更新鼠标指针状态
        QApplication::setOverrideCursor(Qt::WaitCursor);
        if (!QFileInfo(file).completeSuffix().length())
            file += QString(".bmp");

        const TCollection_AsciiString anUtf8Path(file.toUtf8().data());

        bool res = myView->dump(anUtf8Path.ToCString());
        QApplication::restoreOverrideCursor();


        if (!res)
        {
            std::cout << tr("图像导出错误").toStdString() << std::endl;
        }
    }
}

void MainWindow::onSelectionChanged()
{
    updateDisplaymodeActionEnableStat(myContext->NbSelected());

    // 发出信号，便于其他的slots进行处理
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())
    {
        Handle_AIS_Shape shape = Handle_AIS_Shape::DownCast(myContext->SelectedInteractive());
        std::cout << QString("面%1被选择").arg(shape->Shape().HashCode(SHAPE_MAXHASHCODE)).toStdString();
    }

}

Handle(V3d_Viewer) MainWindow::Viewer(const Standard_ExtString    theName,
                                      const Standard_CString      theDomain,
                                      const Standard_Real         theViewSize,
                                      const V3d_TypeOfOrientation theViewProj,
                                      const Standard_Boolean      theComputedMode,
                                      const Standard_Boolean      theDefaultComputedMode)
{
    // 配置图形驱动程序，比如Nvidia或者Intel或者Radeon
    static Handle(OpenGl_GraphicDriver) aGraphicDriver;

    if (aGraphicDriver.IsNull())
    {
        Handle(Aspect_DisplayConnection) aDisplayConnection;
#if !defined(_WIN32) && !defined(__WIN32__) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX))
        aDisplayConnection = new Aspect_DisplayConnection(OSD_Environment("DISPLAY").Value());
#endif
        aGraphicDriver = new OpenGl_GraphicDriver(aDisplayConnection);
    }

    Handle(V3d_Viewer) aViewer = new V3d_Viewer(aGraphicDriver);
    aViewer->SetDefaultViewSize(theViewSize);
    aViewer->SetDefaultViewProj(theViewProj);
    aViewer->SetComputedMode(theComputedMode);
    aViewer->SetDefaultComputedMode(theDefaultComputedMode);
    return aViewer;
}

void MainWindow::updateDisplaymodeActionEnableStat(const int nSels)
{
    bool OneOrMoreInShading   = false;
    bool OneOrMoreInWireframe = false;
    if (nSels)
    {
        // 如果有对象被选中
        for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())
        {
            if (myContext->IsDisplayed(myContext->SelectedInteractive(), 1))
                OneOrMoreInShading = true;
            if (myContext->IsDisplayed(myContext->SelectedInteractive(), 0))
                OneOrMoreInWireframe = true;
        }
        myView->getDisplaymodeAction(ModelView::ToolWireframeId)->setEnabled(OneOrMoreInShading);
        myView->getDisplaymodeAction(ModelView::ToolShadingId)->setEnabled(OneOrMoreInWireframe);
        //        myView->getDisplaymodeAction(ModelView::ToolMaterialId)->setEnabled(true);
        //        myView->getDisplaymodeAction(ModelView::ToolTransparencyId)->setEnabled(OneOrMoreInShading);
        myView->getDisplaymodeAction(ModelView::ToolDeleteId)->setEnabled(true);
    }
    else
    {
        // 如果没有对象被选中
        myView->getDisplaymodeAction(ModelView::ToolWireframeId)->setEnabled(false);
        myView->getDisplaymodeAction(ModelView::ToolShadingId)->setEnabled(false);
        //        myView->getDisplaymodeAction(ModelView::ToolMaterialId)->setEnabled(false);
        //        myView->getDisplaymodeAction(ModelView::ToolTransparencyId)->setEnabled(false);
        myView->getDisplaymodeAction(ModelView::ToolDeleteId)->setEnabled(false);
    }
}

void MainWindow::createViewActions()
{
    // populate a tool bar with some actions
    QToolBar *aToolBar = addToolBar(tr("View Operations"));
    aToolBar->addActions(myView->getViewActions());

    aToolBar->toggleViewAction()->setVisible(false);
    myView->getViewAction(ModelView::ViewHlrOffId)->setChecked(true);
}

void MainWindow::createRaytraceActions()
{
    // 增加一个toolbar，同时添加一些actions
    QToolBar *aToolbar = addToolBar(tr("Ray-tracing Options"));
    aToolbar->addActions(myView->getRaytraceActions());

    aToolbar->toggleViewAction()->setVisible(true);
    myView->getRaytraceAction(ModelView::ToolRaytracingId)->setChecked(false);
    myView->getRaytraceAction(ModelView::ToolShadowsId)->setChecked(true);
    myView->getRaytraceAction(ModelView::ToolReflectionsId)->setChecked(false);
    myView->getRaytraceAction(ModelView::ToolAntialiasingId)->setChecked(false);
}

void MainWindow::createDisplaymodeActions()
{
    QToolBar *aToolbar = addToolBar(tr("Shape Operations"));
    aToolbar->addActions(myView->getDisplaymodeActions());

    aToolbar->toggleViewAction()->setVisible(true);
    // 处理一下初始化后没有物体被选中的问题
    onSelectionChanged();
}
