#if !defined _WIN32
#define QT_CLEAN_NAMESPACE /* avoid definition of INT32 and INT8 */
#endif

#include "ModelView.h"
#include "OcctWindow.h"

#include <QApplication>
#include <QColorDialog>
#include <QCursor>
#include <QFileDialog>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRubberBand>
#include <QStyleFactory>

#include <Standard_WarningsDisable.hxx>
#include <Standard_WarningsRestore.hxx>
#if !defined(_WIN32) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX)) && QT_VERSION < 0x050000
#include <QX11Info>
#endif

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Graphic3d_TextureEnv.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <StdSelect_FaceFilter.hxx>
#include <TopExp_Explorer.hxx>


namespace
{
    // 用于获取Qt鼠标按键指令
    //! Map Qt buttons bitmask to virtual keys.
    Aspect_VKeyMouse qtMouseButtons2VKeys(Qt::MouseButtons theButtons)
    {
        Aspect_VKeyMouse aButtons = Aspect_VKeyMouse_NONE;
        if ((theButtons & Qt::LeftButton) != 0)
        {
            aButtons |= Aspect_VKeyMouse_LeftButton;
        }
        if ((theButtons & Qt::MiddleButton) != 0)
        {
            aButtons |= Aspect_VKeyMouse_MiddleButton;
        }
        if ((theButtons & Qt::RightButton) != 0)
        {
            aButtons |= Aspect_VKeyMouse_RightButton;
        }
        return aButtons;
    }

    // 用于处理Qt功能组合键到Aspect_VKeyMouse对象
    //! Map Qt mouse modifiers bitmask to virtual keys.
    Aspect_VKeyFlags qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers)
    {
        Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
        if ((theModifiers & Qt::ShiftModifier) != 0)
        {
            aFlags |= Aspect_VKeyFlags_SHIFT;
        }
        if ((theModifiers & Qt::ControlModifier) != 0)
        {
            aFlags |= Aspect_VKeyFlags_CTRL;
        }
        if ((theModifiers & Qt::AltModifier) != 0)
        {
            aFlags |= Aspect_VKeyFlags_ALT;
        }
        return aFlags;
    }
}    // namespace

static QCursor *defCursor     = NULL;
static QCursor *handCursor    = NULL;
static QCursor *panCursor     = NULL;
static QCursor *globPanCursor = NULL;
static QCursor *zoomCursor    = NULL;
static QCursor *rotCursor     = NULL;

ModelView::ModelView(const Handle(AIS_InteractiveContext) & theContext, bool with_viewcube, QWidget *parent)
    : QWidget(parent)
    , myIsRaytracing(false)
    , myIsShadowsEnabled(true)
    , myIsReflectionsEnabled(false)
    , myIsAntialiasingEnabled(false)
    , myBackMenu(NULL)
{
#if !defined(_WIN32) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX)) && QT_VERSION < 0x050000
    XSynchronize(x11Info().display(), true);
#endif
    myContext = theContext;

    myCurZoom = 0;
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    myDefaultGestures = myMouseGestureMap;
    myCurrentMode     = CurAction3d_Nothing;
    setMouseTracking(true);

    if (with_viewcube)
    {
        Handle(AIS_ViewCube) viewcube = new AIS_ViewCube();
        myContext->Display(viewcube, Standard_False);
    }

    myContext->SelectionManager();

    initViewActions();
    initCursors();

    setBackgroundRole(QPalette::NoRole);    //NoBackground );
    // set focus policy to threat QContextMenuEvent from keyboard
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    init();
    initSelectionModeActions();
}

ModelView::~ModelView()
{
    delete myBackMenu;
}

void ModelView::init()
{
    if (myV3dView.IsNull())
        myV3dView = myContext->CurrentViewer()->CreateView();

    Handle(OcctWindow) hWnd = new OcctWindow(this);
    myV3dView->SetWindow(hWnd);
    if (!hWnd->IsMapped())
    {
        hWnd->Map();
    }

    SetAllowRotation(Standard_True);
    myV3dView->SetBackgroundColor(Quantity_NOC_BLACK);
    myV3dView->MustBeResized();

    if (myIsRaytracing)
        myV3dView->ChangeRenderingParams().Method = Graphic3d_RM_RAYTRACING;
}

// 页面绘制事件
void ModelView::paintEvent(QPaintEvent *)
{
    //  QApplication::syncX();
    myV3dView->InvalidateImmediate();
    FlushViewEvents(myContext, myV3dView, true);
}

void ModelView::resizeEvent(QResizeEvent *)
{
    //  QApplication::syncX();
    if (!myV3dView.IsNull())
    {
        myV3dView->MustBeResized();
    }
}

/// \brief 复写被选择的物体变化后，用于处理的事件
void ModelView::OnSelectionChanged(const Handle(AIS_InteractiveContext) & ctx,
                                   const Handle(V3d_View) & theView)
{
    Q_UNUSED(ctx);
    Q_UNUSED(theView);
    // 遍历整个视图中被选中的object
    //    for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
    //    {
    //        //被选择物体的交互对象
    //        Handle(AIS_InteractiveObject) anIO = ctx->SelectedInteractive();

    //        ctx->SetDisplayMode(ctx->SelectedInteractive(), AIS_WireFrame, false);
    //    }

    //    ctx->UpdateCurrentViewer();

    //遍历AIS的所有对象
    //    AIS_ListOfInteractive aList;
    //    ctx->DisplayedObjects(aList);

    //    Standard_Integer nFace = 0;
    //    for (Handle(AIS_InteractiveObject) obj : aList)
    //    {
    //        if (obj->Type() == AIS_KOI_Shape)
    //        {
    //            //            obj->Delete();
    //            Handle_AIS_Shape as = Handle_AIS_Shape::DownCast(obj);
    //            for (TopExp_Explorer exp(as->Shape(), TopAbs_FACE); exp.More(); exp.Next())
    //            {
    //                Handle_AIS_Shape facex = new AIS_Shape(exp.Current());
    //                facex->SetDisplayMode(AIS_Shaded);
    //                ctx->Display(facex, false);
    //                nFace++;
    //            }
    //            ctx->Remove(obj, false);
    //        }
    //        else
    //        {
    //            //            if (typeid(obj) == typeid(AIS_ViewCube))
    //            //                dbginfo std::cout << "obj类型是viewcube" << std::endl;
    //        }
    //    }

    //    dbginfo std::cout<<"faces="<<nFace<<std::endl;

    // todo 增加当界面物体被单击后，在材质编辑界面展开该处内容


    // 调用对应的槽接口
    //    for (ctx->InitSelected(); ctx->MoreSelected(); ctx->NextSelected())
    //    {
    //        //被选择物体的交互对象
    //        Handle_AIS_Shape anShape = Handle_AIS_Shape::DownCast(ctx->SelectedInteractive());


    //        ctx->SetDisplayMode(ctx->SelectedInteractive(), AIS_WireFrame, false);
    //    }

    //    ctx->UpdateCurrentViewer();

    //    dbginfo LOGGERINFO("有对象被选择，正在发出");
    emit selectionChanged();
}



void ModelView::fitAll()
{
    myV3dView->FitAll();
    myV3dView->ZFitAll();
    myV3dView->Redraw();
}

void ModelView::fitArea()
{
    setCurrentAction(CurAction3d_WindowZooming);
}

void ModelView::zoom()
{
    setCurrentAction(CurAction3d_DynamicZooming);
}

void ModelView::pan()
{
    setCurrentAction(CurAction3d_DynamicPanning);
}

void ModelView::rotation()
{
    setCurrentAction(CurAction3d_DynamicRotation);
}

void ModelView::globalPan()
{
    // save the current zoom value
    myCurZoom = myV3dView->Scale();
    // Do a Global Zoom
    myV3dView->FitAll();
    // Set the moderotation
    setCurrentAction(CurAction3d_GlobalPanning);
}

void ModelView::front()
{
    myV3dView->SetProj(V3d_Yneg);
}

void ModelView::back()
{
    myV3dView->SetProj(V3d_Ypos);
}

void ModelView::top()
{
    myV3dView->SetProj(V3d_Zpos);
}

void ModelView::bottom()
{
    myV3dView->SetProj(V3d_Zneg);
}

void ModelView::left()
{
    myV3dView->SetProj(V3d_Xneg);
}

void ModelView::right()
{
    myV3dView->SetProj(V3d_Xpos);
}

void ModelView::axo()
{
    myV3dView->SetProj(V3d_XposYnegZpos);
}

void ModelView::reset()
{
    myV3dView->Reset();
}

void ModelView::hlrOff()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    myV3dView->SetComputedMode(Standard_False);
    myV3dView->Redraw();
    QAction *aShadingAction = getDisplaymodeAction(ToolShadingId);
    aShadingAction->setEnabled(true);
    QAction *aWireframeAction = getDisplaymodeAction(ToolWireframeId);
    aWireframeAction->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void ModelView::hlrOn()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    myV3dView->SetComputedMode(Standard_True);
    myV3dView->Redraw();
    QAction *aShadingAction = getDisplaymodeAction(ToolShadingId);
    aShadingAction->setEnabled(false);
    QAction *aWireframeAction = getDisplaymodeAction(ToolWireframeId);
    aWireframeAction->setEnabled(false);
    QApplication::restoreOverrideCursor();
}

void ModelView::SetRaytracedShadows(bool theState)
{
    myV3dView->ChangeRenderingParams().IsShadowEnabled = theState;
    myIsShadowsEnabled                                 = theState;
    myContext->UpdateCurrentViewer();
}

void ModelView::SetRaytracedReflections(bool theState)
{
    myV3dView->ChangeRenderingParams().IsReflectionEnabled = theState;
    myIsReflectionsEnabled                                 = theState;
    myContext->UpdateCurrentViewer();
}

void ModelView::onRaytraceAction()
{
    QAction *aSentBy = (QAction *)sender();

    if (aSentBy == getRaytraceAction(ToolRaytracingId))
    {
        bool aState = getRaytraceAction(ToolRaytracingId)->isChecked();

        QApplication::setOverrideCursor(Qt::WaitCursor);
        if (aState)
            EnableRaytracing();
        else
            DisableRaytracing();
        QApplication::restoreOverrideCursor();
    }

    if (aSentBy == getRaytraceAction(ToolShadowsId))
    {
        bool aState = getRaytraceAction(ToolShadowsId)->isChecked();
        SetRaytracedShadows(aState);
    }

    if (aSentBy == getRaytraceAction(ToolReflectionsId))
    {
        bool aState = getRaytraceAction(ToolReflectionsId)->isChecked();
        SetRaytracedReflections(aState);
    }

    if (aSentBy == getRaytraceAction(ToolAntialiasingId))
    {
        bool aState = getRaytraceAction(ToolAntialiasingId)->isChecked();
        SetRaytracedAntialiasing(aState);
    }
}

void ModelView::onWireframe()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())
        myContext->SetDisplayMode(myContext->SelectedInteractive(), AIS_WireFrame, false);
    myContext->UpdateCurrentViewer();
    OnSelectionChanged();
    QApplication::restoreOverrideCursor();
}

void ModelView::onShading()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())
        myContext->SetDisplayMode(myContext->SelectedInteractive(), AIS_Shaded, false);
    myContext->UpdateCurrentViewer();
    OnSelectionChanged();
    QApplication::restoreOverrideCursor();
}

//void ModelView::onMaterial()
//{
//    DialogMaterial *m = new DialogMaterial();
//    connect(m, SIGNAL(sendMaterialChanged(int)), this, SLOT(onMaterial(int)));
//    m->exec();
//}

//void ModelView::onMaterial(int theMaterial)
//{
//    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())
//        myContext->SetMaterial(myContext->SelectedInteractive(),
//                               (Graphic3d_NameOfMaterial)theMaterial, Standard_False);
//    myContext->UpdateCurrentViewer();
//}

//void ModelView::onTransparency()
//{
//    DialogTransparency *aDialog = new DialogTransparency();
//    connect(aDialog, SIGNAL(sendTransparencyChanged(int)), this, SLOT(onTransparency(int)));
//    aDialog->exec();
//}

void ModelView::onTransparency(int theTrans)
{
    for (myContext->InitSelected(); myContext->MoreSelected(); myContext->NextSelected())
        myContext->SetTransparency(myContext->SelectedInteractive(),
                                   ((Standard_Real)theTrans) / 10.0, Standard_False);
    myContext->UpdateCurrentViewer();
}

void ModelView::onDelete()
{
    myContext->EraseSelected(Standard_False);
    myContext->ClearSelected(Standard_False);
    myContext->UpdateCurrentViewer();

    // 已选择部分更新
    OnSelectionChanged();
}



void ModelView::onToolAction()
{
    QAction *sentBy = (QAction *)sender();

    if (sentBy == getDisplaymodeAction(ToolWireframeId))
        onWireframe();

    if (sentBy == getDisplaymodeAction(ToolShadingId))
        onShading();

    //    if (sentBy == getDisplaymodeAction(ToolMaterialId))
    //        onMaterial();

    //    if (sentBy == getDisplaymodeAction(ToolTransparencyId))
    //        onTransparency();

    if (sentBy == getDisplaymodeAction(ToolDeleteId))
        onDelete();
}

void ModelView::SetRaytracedAntialiasing(bool theState)
{
    myV3dView->ChangeRenderingParams().IsAntialiasingEnabled = theState;

    myIsAntialiasingEnabled = theState;

    myContext->UpdateCurrentViewer();
}

void ModelView::EnableRaytracing()
{
    if (!myIsRaytracing)
        myV3dView->ChangeRenderingParams().Method = Graphic3d_RM_RAYTRACING;

    myIsRaytracing = true;

    myContext->UpdateCurrentViewer();
}

void ModelView::DisableRaytracing()
{
    if (myIsRaytracing)
        myV3dView->ChangeRenderingParams().Method = Graphic3d_RM_RASTERIZATION;

    myIsRaytracing = false;

    myContext->UpdateCurrentViewer();
}

void ModelView::updateToggled(bool isOn)
{
    QAction *sentBy = (QAction *)sender();

    if (!isOn)
        return;

    foreach (QAction *anAction, myViewActions)
    {
        if ((anAction == getViewAction(ViewFitAreaId)) ||
            (anAction == getViewAction(ViewZoomId)) ||
            (anAction == getViewAction(ViewPanId)) ||
            (anAction == getViewAction(ViewGlobalPanId)) ||
            (anAction == getViewAction(ViewRotationId)))
        {
            if (anAction && (anAction != sentBy))
            {
                anAction->setCheckable(true);
                anAction->setChecked(false);
            }
            else
            {
                if (sentBy == getViewAction(ViewFitAreaId))
                    setCursor(*handCursor);
                else if (sentBy == getViewAction(ViewZoomId))
                    setCursor(*zoomCursor);
                else if (sentBy == getViewAction(ViewPanId))
                    setCursor(*panCursor);
                else if (sentBy == getViewAction(ViewGlobalPanId))
                    setCursor(*globPanCursor);
                else if (sentBy == getViewAction(ViewRotationId))
                    setCursor(*rotCursor);
                else
                    setCursor(*defCursor);

                sentBy->setCheckable(false);
            }
        }
    }
}

void ModelView::initCursors()
{
    if (!defCursor)
        defCursor = new QCursor(Qt::ArrowCursor);
    if (!handCursor)
        handCursor = new QCursor(Qt::PointingHandCursor);
    if (!panCursor)
        panCursor = new QCursor(Qt::SizeAllCursor);
    if (!globPanCursor)
        globPanCursor = new QCursor(Qt::CrossCursor);
    if (!zoomCursor)
        zoomCursor = new QCursor(QPixmap(QString::fromUtf8(":/common/res/common/view_zoom.png")));
    if (!rotCursor)
        rotCursor = new QCursor(QPixmap(QString::fromUtf8(":/common/res/common/view_rotate.png")));
}

/// \brief 返回视角控制相关的Actions
QList<QAction *> ModelView::getViewActions()
{
    initViewActions();
    return myViewActions.values();
}

/// \brief 返回观察页面渲染方面的Actions
QList<QAction *> ModelView::getRaytraceActions()
{
    initRaytraceActions();
    return myRaytraceActions.values();
}

/// \brief 返回显示模式方面的Actions
QList<QAction *> ModelView::getDisplaymodeActions()
{
    initDisplaymodeActions();
    return myDisplaymodesActions.values();
}


/*!
  Get paint engine for the OpenGL viewer. [ virtual public ]
*/
QPaintEngine *ModelView::paintEngine() const
{
    return 0;
}

void ModelView::initViewActions()
{
    if (!myViewActions.empty())
        return;

    QAction *a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_fitall.png")),
                    QObject::tr("FitAll"), this);
    a->setToolTip(QObject::tr("FitAll"));
    a->setStatusTip(QObject::tr("FitAll"));
    connect(a, SIGNAL(triggered()), this, SLOT(fitAll()));
    myViewActions[ViewFitAllId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_fitarea.png")),
                    QObject::tr("FitArea"), this);
    a->setToolTip(QObject::tr("FitArea"));
    a->setStatusTip(QObject::tr("FitArea"));
    connect(a, SIGNAL(triggered()), this, SLOT(fitArea()));
    a->setCheckable(true);
    connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
    myViewActions[ViewFitAreaId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_zoom.png")),
                    QObject::tr("Dynamic Zooming"), this);
    a->setToolTip(QObject::tr("Dynamic Zooming"));
    a->setStatusTip(QObject::tr("Dynamic Zooming"));
    connect(a, SIGNAL(triggered()), this, SLOT(zoom()));
    a->setCheckable(true);
    connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
    myViewActions[ViewZoomId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_pan.png")),
                    QObject::tr("Dynamic Panning"), this);
    a->setToolTip(QObject::tr("Dynamic Panning"));
    a->setStatusTip(QObject::tr("Dynamic Panning"));
    connect(a, SIGNAL(triggered()), this, SLOT(pan()));
    a->setCheckable(true);
    connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
    myViewActions[ViewPanId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_glpan.png")),
                    QObject::tr("Global Panning"), this);
    a->setToolTip(QObject::tr("Global Panning"));
    a->setStatusTip(QObject::tr("Global Panning"));
    connect(a, SIGNAL(triggered()), this, SLOT(globalPan()));
    a->setCheckable(true);
    connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
    myViewActions[ViewGlobalPanId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_front.png")),
                    QObject::tr("Front"), this);
    a->setToolTip(QObject::tr("Front"));
    a->setStatusTip(QObject::tr("Front"));
    connect(a, SIGNAL(triggered()), this, SLOT(front()));
    myViewActions[ViewFrontId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_back.png")),
                    QObject::tr("Back"), this);
    a->setToolTip(QObject::tr("Back"));
    a->setStatusTip(QObject::tr("Back"));
    connect(a, SIGNAL(triggered()), this, SLOT(back()));
    myViewActions[ViewBackId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_top.png")),
                    QObject::tr("Top"), this);
    a->setToolTip(QObject::tr("Top"));
    a->setStatusTip(QObject::tr("Top"));
    connect(a, SIGNAL(triggered()), this, SLOT(top()));
    myViewActions[ViewTopId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_bottom.png")),
                    QObject::tr("Bottom"), this);
    a->setToolTip(QObject::tr("Bottom"));
    a->setStatusTip(QObject::tr("Bottom"));
    connect(a, SIGNAL(triggered()), this, SLOT(bottom()));
    myViewActions[ViewBottomId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_left.png")),
                    QObject::tr("Left"), this);
    a->setToolTip(QObject::tr("Left"));
    a->setStatusTip(QObject::tr("Left"));
    connect(a, SIGNAL(triggered()), this, SLOT(left()));
    myViewActions[ViewLeftId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_right.png")),
                    QObject::tr("Right"), this);
    a->setToolTip(QObject::tr("Right"));
    a->setStatusTip(QObject::tr("Right"));
    connect(a, SIGNAL(triggered()), this, SLOT(right()));
    myViewActions[ViewRightId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_axo.png")),
                    QObject::tr("Axo"), this);
    a->setToolTip(QObject::tr("Axo"));
    a->setStatusTip(QObject::tr("Axo"));
    connect(a, SIGNAL(triggered()), this, SLOT(axo()));
    myViewActions[ViewAxoId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_rotate.png")),
                    QObject::tr("Dynamic Rotation"), this);
    a->setToolTip(QObject::tr("Dynamic Rotation"));
    a->setStatusTip(QObject::tr("Dynamic Rotation"));
    connect(a, SIGNAL(triggered()), this, SLOT(rotation()));
    a->setCheckable(true);
    connect(a, SIGNAL(toggled(bool)), this, SLOT(updateToggled(bool)));
    myViewActions[ViewRotationId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_reset.png")),
                    QObject::tr("Reset"), this);
    a->setToolTip(QObject::tr("Reset"));
    a->setStatusTip(QObject::tr("Reset"));
    connect(a, SIGNAL(triggered()), this, SLOT(reset()));
    myViewActions[ViewResetId] = a;

    QActionGroup *ag = new QActionGroup(this);

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_comp_off.png")),
                    QObject::tr("Hidden Off"), this);
    a->setToolTip(QObject::tr("Hidden Off"));
    a->setStatusTip(QObject::tr("Hidden Off"));
    connect(a, SIGNAL(triggered()), this, SLOT(hlrOff()));
    a->setCheckable(true);
    ag->addAction(a);
    myViewActions[ViewHlrOffId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/view_comp_on.png")),
                    QObject::tr("Hidden On"), this);
    a->setToolTip(QObject::tr("Hidden On"));
    a->setStatusTip(QObject::tr("Hidden On"));
    connect(a, SIGNAL(triggered()), this, SLOT(hlrOn()));
    a->setCheckable(true);
    ag->addAction(a);
    myViewActions[ViewHlrOnId] = a;
}

void ModelView::initRaytraceActions()
{
    if (!myRaytraceActions.isEmpty())
        return;

    QAction *a;
    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/raytracing.png")),
                    QObject::tr("Enable Ray-tracing"), this);
    a->setToolTip(QObject::tr("Enable Ray-tracing"));
    a->setStatusTip(QObject::tr("Enable Ray-tracing"));
    a->setCheckable(true);
    a->setChecked(false);
    connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
    myRaytraceActions[ToolRaytracingId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/shadows.png")),
                    QObject::tr("Enable Shadows"), this);
    a->setToolTip(QObject::tr("Enable Shadows"));
    a->setStatusTip(QObject::tr("Enable Shadows"));
    a->setCheckable(true);
    a->setChecked(true);
    connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
    myRaytraceActions[ToolShadowsId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/reflections.png")),
                    QObject::tr("Enable Reflections"), this);
    a->setToolTip(QObject::tr("Enable Reflections"));
    a->setStatusTip(QObject::tr("Enable Reflections"));
    a->setCheckable(true);
    a->setChecked(false);
    connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
    myRaytraceActions[ToolReflectionsId] = a;

    a = new QAction(QPixmap(QString::fromUtf8(":/common/res/common/antialiasing.png")),
                    QObject::tr("Enable Anti-aliasing"), this);
    a->setToolTip(QObject::tr("Enable Anti-aliasing"));
    a->setStatusTip(QObject::tr("Enable Anti-aliasing"));
    a->setCheckable(true);
    a->setChecked(false);
    connect(a, SIGNAL(triggered()), this, SLOT(onRaytraceAction()));
    myRaytraceActions[ToolAntialiasingId] = a;
}

void ModelView::initDisplaymodeActions()
{
    if (!myDisplaymodesActions.empty())
        return;

    QString  icon_prefix(":/common/res/common/");
    QAction *a;

    a = new QAction(QPixmap(icon_prefix + "tool_wireframe.png"), QObject::tr("Wireframe"), this);
    a->setToolTip(QObject::tr("Wireframe"));
    a->setStatusTip(QObject::tr("Wireframe"));
    connect(a, SIGNAL(triggered()), this, SLOT(onToolAction()));
    myDisplaymodesActions[ToolWireframeId] = a;

    a = new QAction(QPixmap(icon_prefix + "tool_shading.png"), QObject::tr("Shadows"), this);
    a->setToolTip(QObject::tr("Shadows"));
    a->setStatusTip(QObject::tr("Shadows"));
    connect(a, SIGNAL(triggered()), this, SLOT(onToolAction()));
    myDisplaymodesActions[ToolShadingId] = a;

    //    a = new QAction(QPixmap(icon_prefix + "tool_material.png"), QObject::tr("MNU_TOOL_MATER"), this);
    //    a->setToolTip(QObject::tr("TBR_TOOL_MATER"));
    //    a->setStatusTip(QObject::tr("TBR_TOOL_MATER"));
    //    connect(a, SIGNAL(triggered()), this, SLOT(onToolAction()));
    //    myDisplaymodesActions[ToolMaterialId] = a;

    //    a = new QAction(QPixmap(icon_prefix + "tool_transparency.png"), QObject::tr("MNU_TOOL_TRANS"), this);
    //    a->setToolTip(QObject::tr("TBR_TOOL_TRANS"));
    //    a->setStatusTip(QObject::tr("TBR_TOOL_TRANS"));
    //    connect(a, SIGNAL(triggered()), this, SLOT(onToolAction()));
    //    myDisplaymodesActions[ToolTransparencyId] = a;

    a = new QAction(QPixmap(icon_prefix + "tool_delete.png"), QObject::tr("Delete"), this);
    a->setToolTip(QObject::tr("Delete"));
    a->setStatusTip(QObject::tr("Delete"));
    a->setShortcut(QKeySequence::Delete);
    connect(a, SIGNAL(triggered()), this, SLOT(onToolAction()));
    myDisplaymodesActions[ToolDeleteId] = a;
}

void ModelView::initSelectionModeActions()
{
    if (!mySelectionModeActions.empty())
        return;

    QAction *a;

    a = new QAction(QObject::tr("Select Face"), this);
    a->setToolTip(QObject::tr("面选择模式"));
    a->setStatusTip(QObject::tr("面选择模式"));
    connect(a, SIGNAL(triggered()), this, SLOT(onSelectionModeChange()));
    mySelectionModeActions[TopAbs_FACE] = a;

    a = new QAction(QObject::tr("Select Shape"), this);
    a->setToolTip(QObject::tr("Shape选择模式"));
    a->setStatusTip(QObject::tr("Shape选择模式"));
    connect(a, SIGNAL(triggered()), this, SLOT(onSelectionModeChange()));
    mySelectionModeActions[TopAbs_SHAPE] = a;
}

void ModelView::activateCursor(const CurrentAction3d mode)
{
    switch (mode)
    {
        case CurAction3d_DynamicPanning:
            setCursor(*panCursor);
            break;
        case CurAction3d_DynamicZooming:
            setCursor(*zoomCursor);
            break;
        case CurAction3d_DynamicRotation:
            setCursor(*rotCursor);
            break;
        case CurAction3d_GlobalPanning:
            setCursor(*globPanCursor);
            break;
        case CurAction3d_WindowZooming:
            setCursor(*handCursor);
            break;
        case CurAction3d_Nothing:
        default:
            setCursor(*defCursor);
            break;
    }
}


void ModelView::mousePressEvent(QMouseEvent *theEvent)
{
    const Graphic3d_Vec2i  aPnt(theEvent->pos().x(), theEvent->pos().y());
    const Aspect_VKeyFlags aFlags = qtMouseModifiers2VKeys(theEvent->modifiers());
    if (!myV3dView.IsNull() && UpdateMouseButtons(aPnt,
                                                  qtMouseButtons2VKeys(theEvent->buttons()),
                                                  aFlags,
                                                  false))
    {
        updateView();
    }
    myClickPos = aPnt;
}

void ModelView::mouseReleaseEvent(QMouseEvent *theEvent)
{
    const Graphic3d_Vec2i  aPnt(theEvent->pos().x(), theEvent->pos().y());
    const Aspect_VKeyFlags aFlags = qtMouseModifiers2VKeys(theEvent->modifiers());
    if (!myV3dView.IsNull() && UpdateMouseButtons(aPnt,
                                                  qtMouseButtons2VKeys(theEvent->buttons()),
                                                  aFlags,
                                                  false))
    {
        updateView();
    }

    if (myCurrentMode == CurAction3d_GlobalPanning)
    {
        myV3dView->Place(aPnt.x(), aPnt.y(), myCurZoom);
    }
    if (myCurrentMode != CurAction3d_Nothing)
    {
        setCurrentAction(CurAction3d_Nothing);
    }
    if (theEvent->button() == Qt::RightButton && (aFlags & Aspect_VKeyFlags_CTRL) == 0 && (myClickPos - aPnt).cwiseAbs().maxComp() <= 4)
    {
        Popup(aPnt.x(), aPnt.y());
    }
}

void ModelView::mouseMoveEvent(QMouseEvent *theEvent)
{
    const Graphic3d_Vec2i aNewPos(theEvent->pos().x(), theEvent->pos().y());
    if (!myV3dView.IsNull() && UpdateMousePosition(aNewPos,
                                                   qtMouseButtons2VKeys(theEvent->buttons()),
                                                   qtMouseModifiers2VKeys(theEvent->modifiers()),
                                                   false))
    {
        updateView();
    }
}

//==============================================================================
//function : wheelEvent
//purpose  :
//==============================================================================
void ModelView::wheelEvent(QWheelEvent *theEvent)
{
    const Graphic3d_Vec2i aPos(theEvent->position().x(), theEvent->position().y());

    if (!myV3dView.IsNull() && UpdateZoom(Aspect_ScrollDelta(aPos, theEvent->angleDelta().y() / 8)))
    {
        updateView();
    }
}

// =======================================================================
// function : updateView
// purpose  : 更新界面
// =======================================================================
void ModelView::updateView()
{
    update();
}

// 按照不同的操作模式绑定鼠标按钮
void ModelView::defineMouseGestures()
{
    myMouseGestureMap.Clear();
    AIS_MouseGesture aRot = AIS_MouseGesture_RotateOrbit;
    activateCursor(myCurrentMode);
    switch (myCurrentMode)
    {
        case CurAction3d_Nothing: {
            noActiveActions();
            myMouseGestureMap = myDefaultGestures;
            break;
        }
        case CurAction3d_DynamicZooming: {
            myMouseGestureMap.Bind(Aspect_VKeyMouse_LeftButton, AIS_MouseGesture_Zoom);
            break;
        }
        case CurAction3d_GlobalPanning: {
            break;
        }
        case CurAction3d_WindowZooming: {
            myMouseGestureMap.Bind(Aspect_VKeyMouse_LeftButton, AIS_MouseGesture_ZoomWindow);
            break;
        }
        case CurAction3d_DynamicPanning: {
            myMouseGestureMap.Bind(Aspect_VKeyMouse_LeftButton, AIS_MouseGesture_Pan);
            break;
        }
        case CurAction3d_DynamicRotation: {
            myMouseGestureMap.Bind(Aspect_VKeyMouse_LeftButton, aRot);
            break;
        }
    }
}

/// \brief 处理显示界面的右键弹出菜单
void ModelView::Popup(const int /*x*/, const int /*y*/)
{
    if (myContext->NbSelected())
    {
        QMenu *myToolMenu = new QMenu(0);
        myToolMenu->addAction(getDisplaymodeAction(ModelView::ToolWireframeId));
        myToolMenu->addAction(getDisplaymodeAction(ModelView::ToolShadingId));

        myToolMenu->addAction(getDisplaymodeAction(ModelView::ToolTransparencyId));
        myToolMenu->addAction(getDisplaymodeAction(ModelView::ToolDeleteId));


        // 添加目标物体的选择模式
        //        myToolMenu->addSeparator();    //添加一个分割线
        //        myToolMenu->addAction(getSelectionModeAction(TopAbs_FACE));
        //        myToolMenu->addAction(getSelectionModeAction(TopAbs_SHAPE));

        addItemInPopup(myToolMenu);
        myToolMenu->exec(QCursor::pos());
        delete myToolMenu;
    }
    else
    {
        if (!myBackMenu)
        {
            myBackMenu = new QMenu(0);

            QAction *a = new QAction(QObject::tr("Change Background"), this);
            a->setToolTip(QObject::tr("Change Background"));
            connect(a, SIGNAL(triggered()), this, SLOT(onBackground()));
            myBackMenu->addAction(a);
            addItemInPopup(myBackMenu);

            a = new QAction(QObject::tr("Environment Map"), this);
            a->setToolTip(QObject::tr("Environment Map"));
            connect(a, SIGNAL(triggered()), this, SLOT(onEnvironmentMap()));
            a->setCheckable(true);
            a->setChecked(false);
            myBackMenu->addAction(a);
            addItemInPopup(myBackMenu);
        }

        myBackMenu->exec(QCursor::pos());
    }
}

void ModelView::addItemInPopup(QMenu *menu)
{
    Q_UNUSED(menu);
}

void ModelView::noActiveActions()
{
    foreach (QAction *anAction, myViewActions)
    {
        if ((anAction == getViewAction(ViewFitAreaId)) ||
            (anAction == getViewAction(ViewZoomId)) ||
            (anAction == getViewAction(ViewPanId)) ||
            (anAction == getViewAction(ViewGlobalPanId)) ||
            (anAction == getViewAction(ViewRotationId)))
        {
            setCursor(*defCursor);
            anAction->setCheckable(true);
            anAction->setChecked(false);
        }
    }
}

void ModelView::onBackground()
{
    QColor        aColor;
    Standard_Real R1;
    Standard_Real G1;
    Standard_Real B1;
    myV3dView->BackgroundColor(Quantity_TOC_RGB, R1, G1, B1);
    aColor.setRgb((Standard_Integer)(R1 * 255), (Standard_Integer)(G1 * 255), (Standard_Integer)(B1 * 255));

    QColor aRetColor = QColorDialog::getColor(aColor);

    if (aRetColor.isValid())
    {
        R1 = aRetColor.red() / 255.;
        G1 = aRetColor.green() / 255.;
        B1 = aRetColor.blue() / 255.;
        myV3dView->SetBackgroundColor(Quantity_TOC_RGB, R1, G1, B1);
    }
    myV3dView->Redraw();
}

void ModelView::onEnvironmentMap()
{
    if (myBackMenu->actions().at(1)->isChecked())
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                                        tr("All Image Files (*.bmp *.gif *.jpg *.jpeg *.png *.tga)"));

        const TCollection_AsciiString anUtf8Path(fileName.toUtf8().data());

        Handle(Graphic3d_TextureEnv) aTexture = new Graphic3d_TextureEnv(anUtf8Path);

        myV3dView->SetTextureEnv(aTexture);
    }
    else
    {
        myV3dView->SetTextureEnv(Handle(Graphic3d_TextureEnv)());
    }

    myV3dView->Redraw();
}

bool ModelView::dump(Standard_CString theFile)
{
    return myV3dView->Dump(theFile);
}

Handle(V3d_View) & ModelView::getView()
{
    return myV3dView;
}

Handle(AIS_InteractiveContext) & ModelView::getContext()
{
    return myContext;
}

ModelView::CurrentAction3d ModelView::getCurrentMode()
{
    return myCurrentMode;
}
