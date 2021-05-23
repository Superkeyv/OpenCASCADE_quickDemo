#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <QAction>
#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <Standard_WarningsDisable.hxx>
#include <Standard_WarningsRestore.hxx>
#include <V3d_View.hxx>


class ModelView : public QWidget, protected AIS_ViewController
{
    Q_OBJECT

protected:
    enum CurrentAction3d
    {
        CurAction3d_Nothing,
        CurAction3d_DynamicZooming,
        CurAction3d_WindowZooming,
        CurAction3d_DynamicPanning,
        CurAction3d_GlobalPanning,
        CurAction3d_DynamicRotation
    };

public:
    enum ViewAction
    {
        ViewFitAllId,
        ViewFitAreaId,
        ViewZoomId,
        ViewPanId,
        ViewGlobalPanId,
        ViewFrontId,
        ViewBackId,
        ViewTopId,
        ViewBottomId,
        ViewLeftId,
        ViewRightId,
        ViewAxoId,
        ViewRotationId,
        ViewResetId,
        ViewHlrOffId,
        ViewHlrOnId
    };
    enum RaytraceAction
    {
        ToolRaytracingId,
        ToolShadowsId,
        ToolReflectionsId,
        ToolAntialiasingId
    };
    enum DisplaymodeAction
    {
        ToolWireframeId,
        ToolShadingId,
        ToolMaterialId,        //配置材质信息
        ToolTransparencyId,    //配置透明度信息
        ToolDeleteId
    };

    /// \brief 构造一个页面V3dViewer控件，包装成Qwidget可以放在Qt程序中显示
    ///
    /// 在AIS_InteractiveContext的基础上，创建一个Qwidget控件，用于渲染导入的几何对象
    ///
    /// \param theContext， 即AIS_InteractiveContext上下文
    /// \param with_viewcube，是否在viewer中显示ViewCube，也就是视角指示6面体
    /// \param parent，设置ModelView的父对象，用于方便操作父类使用
    /// \return Description for return value
    ModelView(const Handle(AIS_InteractiveContext) & theContext, bool with_viewcube, QWidget *parent);
    ~ModelView();

    virtual void init();

    bool             dump(Standard_CString theFile);
    QList<QAction *> getViewActions();
    QList<QAction *> getRaytraceActions();
    QList<QAction *> getDisplaymodeActions();
    inline QAction * getViewAction(ViewAction a) { return myViewActions[a]; }
    inline QAction * getRaytraceAction(RaytraceAction a) { return myRaytraceActions[a]; }
    inline QAction * getDisplaymodeAction(DisplaymodeAction a) { return myDisplaymodesActions[a]; }
    inline QAction * getSelectionModeAction(TopAbs_ShapeEnum a) { return mySelectionModeActions[a]; }
    void             noActiveActions();
    bool             isShadingMode();

    void EnableRaytracing();
    void DisableRaytracing();

    void SetRaytracedShadows(bool theState);
    void SetRaytracedReflections(bool theState);
    void SetRaytracedAntialiasing(bool theState);

    bool IsRaytracingMode() const { return myIsRaytracing; }
    bool IsShadowsEnabled() const { return myIsShadowsEnabled; }
    bool IsReflectionsEnabled() const { return myIsReflectionsEnabled; }
    bool IsAntialiasingEnabled() const { return myIsAntialiasingEnabled; }

    static QString GetMessages(int type, TopAbs_ShapeEnum aSubShapeType,
                               TopAbs_ShapeEnum aShapeType);
    static QString GetShapeType(TopAbs_ShapeEnum aShapeType);

    Standard_EXPORT static void OnButtonuseraction(int ExerciceSTEP,
                                                   Handle(AIS_InteractiveContext) &);
    Standard_EXPORT static void DoSelection(int Id,
                                            Handle(AIS_InteractiveContext) &);

    /// \brief 追加一个视角Cube，也就是View边角出的立方体
    // QT_DEPRECATED_X("该方法已经被放弃，CubeView的添加是自动实现的")
    // NCollection_Vector<Handle(AIS_InteractiveObject)> &getAIS_Shapes() { return myObjects; }

    virtual QPaintEngine *paintEngine() const override;

signals:
    /// \brief selectionChanged
    ///
    /// 被选择对象改变，需要进行的处理过程。
    ///
    /// \return void
    void selectionChanged();

public slots:
    void fitAll();
    void fitArea();
    void zoom();
    void pan();
    void globalPan();
    void front();
    void back();
    void top();
    void bottom();
    void left();
    void right();
    void axo();
    void rotation();
    void reset();
    void hlrOn();
    void hlrOff();
    void updateToggled(bool);
    void onBackground();
    void onEnvironmentMap();
    void onRaytraceAction();

    void onWireframe();
    void onShading();
    //        void onMaterial();    //配置材质
    //        void onMaterial(int);
    //        void onTransparency();    //配置透明度信息
    void onDelete();

    void onToolAction();

private slots:
    // todo 修改为onTransparencyChanged
    void onTransparency(int);


protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void mousePressEvent(QMouseEvent *) override;
    virtual void mouseReleaseEvent(QMouseEvent *) override;
    virtual void mouseMoveEvent(QMouseEvent *) override;
    virtual void wheelEvent(QWheelEvent *) override;
    virtual void addItemInPopup(QMenu *);

    Handle(V3d_View) & getView();
    Handle(AIS_InteractiveContext) & getContext();
    void            activateCursor(const CurrentAction3d);
    void            Popup(const int x, const int y);    /// \brief 这里是ModelView渲染界面中的右键菜单项
    CurrentAction3d getCurrentMode();
    void            updateView();

    //! Setup mouse gestures. 也就是鼠标操作按键
    void defineMouseGestures();

    //! Set current action.
    void setCurrentAction(CurrentAction3d theAction)
    {
        myCurrentMode = theAction;
        defineMouseGestures();
    }

    //! Handle selection changed event.
    //! 注意，面对象对应一个面。体对象对应一个体
    void OnSelectionChanged(const Handle(AIS_InteractiveContext) & theCtx,
                            const Handle(V3d_View) & theView) Standard_OVERRIDE;
    // 上一个方法的手工重载
    inline void OnSelectionChanged() { OnSelectionChanged(myContext, myV3dView); }

private:
    void initCursors();
    void initViewActions();
    void initRaytraceActions();
    void initDisplaymodeActions();
    void initSelectionModeActions();

private:
    bool myIsRaytracing;
    bool myIsShadowsEnabled;
    bool myIsReflectionsEnabled;
    bool myIsAntialiasingEnabled;

    // 当前页面所维护的V3dView
    Handle(V3d_View) myV3dView;
    Handle(AIS_InteractiveContext) myContext;
    //        NCollection_Vector<Handle(AIS_InteractiveObject)> myObjects;    ///< brief 用于构建View渲染图层的队列
    AIS_MouseGestureMap                myDefaultGestures;
    Graphic3d_Vec2i                    myClickPos;
    CurrentAction3d                    myCurrentMode;
    Standard_Real                      myCurZoom;
    QMap<ViewAction, QAction *>        myViewActions;
    QMap<RaytraceAction, QAction *>    myRaytraceActions;
    QMap<DisplaymodeAction, QAction *> myDisplaymodesActions;
    QMap<TopAbs_ShapeEnum, QAction *>  mySelectionModeActions;

    // todo 等待被使用
    QMenu *myBackMenu;
};


#endif    // MODELVIEW_H
