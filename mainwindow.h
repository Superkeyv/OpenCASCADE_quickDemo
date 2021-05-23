#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Standard_Handle.hxx>
#include <V3d_View.hxx>

class ModelView;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /// \brief 获取图像渲染上下文
    inline Handle(AIS_InteractiveContext) getContext() { return myContext; }
    /// \brief 获取ModelView
    inline Handle(V3d_Viewer) & getV3dViewer() { return myV3dViewer; }


public slots:
    void dump();
    void onSelectionChanged();


private:

    Handle(V3d_Viewer) Viewer(const Standard_ExtString    theName,
                              const Standard_CString      theDomain,
                              const Standard_Real         theViewSize,
                              const V3d_TypeOfOrientation theViewProj,
                              const Standard_Boolean      theComputedMode,
                              const Standard_Boolean      theDefaultComputedMode);

    void updateDisplaymodeActionEnableStat(const int nSels);

protected:
    void createViewActions();
    void createRaytraceActions();
    void createDisplaymodeActions();
    Handle(V3d_Viewer) myV3dViewer;
    Handle(AIS_InteractiveContext) myContext;    /// \brief AIS绘图上下文

    ModelView *myView;
};
#endif    // MAINWINDOW_H
