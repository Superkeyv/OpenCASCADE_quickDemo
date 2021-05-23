#ifndef TEST_GEOM_CPP
#define TEST_GEOM_CPP

#include "mainwindow.h"

#include <QApplication>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <iostream>
#include <math.h>

#include <Eigen/Core>
#include <Eigen/StdList>

#include <BRep_Tool.hxx>               //用于从TopoDS_Shape数据转化为Geom_Surface数据
#include <GCPnts_AbscissaPoint.hxx>    //用于计算Curve的长度
#include <GeomAPI_IntCS.hxx>           //用于计算Curve和Surface的交点
//#include <GeomLProp.hxx>            //用于计算3维目标物体的局部特征，比如法线和曲率
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomLProp_SLProps.hxx>    //专门用于计算目标物体的局部法向量
#include <GeomTools.hxx>            //Geom的相关工具
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <STEPControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>


using namespace std;

class t_brepbuild : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(t_brepbuild);
    CPPUNIT_TEST(t_surface);
    CPPUNIT_TEST_SUITE_END();

public:
    t_brepbuild()
    {
        m.show();
    }
    void setUp() {}
    void tearDown() {}

    void redraw()
    {
        Handle_AIS_InteractiveContext ctx    = m.getContext();
        Handle_V3d_Viewer             viewer = m.getV3dViewer();

        // 获取Shape的选择模式，默认使用的是Face模式
        const int aSubShapeSelMode = AIS_Shape::SelectionMode(TopAbs_FACE);
        for (int i = 1; i <= aSequence->Size(); i++)
        {
            Handle_AIS_Shape aShape = new AIS_Shape(aSequence->Value(i));
            ctx->Display(aShape, false);
            ctx->SetDisplayMode(aShape, AIS_Shaded, false);    /// > brief 配置shape的显示模式为shaded，也就是有表面
            ctx->Activate(aShape, aSubShapeSelMode);           ///< brief 激活shape的子对象选择模式
        }

        ctx->UpdateCurrentViewer();
    }

    void buildSurface(Handle_Geom_Surface &surface)
    {
        Standard_Real     u1, u2, v1, v2;
        Handle_Geom_Plane plane = new Geom_Plane(gp::XOY());    //先提供XOY平面
        plane->Bounds(u1, u2, v1, v2);

    }


    void load_ground(float W, float H)
    {
        Standard_Real u1, u2, v1, v2;
        gp_Pnt        from(W / 2, H / 2, 0);
        gp_Pnt        to(-W / 2, -H / 2, 0);

        TopoDS_Edge  E = BRepBuilderAPI_MakeEdge(from, to);
        TopoDS_Shape S = BRepPrimAPI_MakePrism(E, gp_Vec(0, 0, 100));

        TopoDS_Face aFace = TopoDS::Face(S);

        BRepTools::UVBounds(aFace, u1, u2, v1, v2);
        cout << "u1=" << u1 << " u2=" << u2 << " v1=" << v1 << " v2=" << v2 << endl;

        // ---
        aSequence->Clear();

        aSequence->Append(S);
    }

    void t_surface()
    {
        load_ground(1000, 1000);
        redraw();
    }

private:
    MainWindow m;

    Handle(TopTools_HSequenceOfShape) aSequence = new TopTools_HSequenceOfShape;
};


int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    CppUnit::TextUi::TestRunner   runner;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();

    // 增加测试实例
    CPPUNIT_TEST_SUITE_REGISTRATION(t_brepbuild);

    runner.addTest(registry.makeTest());
    runner.run();

    return a.exec();
}



#endif    // TEST_GEOM_CPP
