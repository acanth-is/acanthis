// depthmapX - spatial network analysis platform
// Copyright (C) 2017, Petros Koutsolampros

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "depthmapX/glview.h"
#include "salalib/linkutils.h"
#include "salalib/geometrygenerators.h"
#include <QMouseEvent>
#include <QCoreApplication>
#include <math.h>

static QRgb colorMerge(QRgb color, QRgb mergecolor)
{
   return (color & 0x006f6f6f) | (mergecolor & 0x00a0a0a0);
}

GLView::GLView(QWidget *parent, QGraphDoc* doc, const QRgb &backgroundColour, const QRgb &foregroundColour)
    : QOpenGLWidget(parent),
      m_eyePosX(0),
      m_eyePosY(0),
      m_background(backgroundColour),
      m_foreground(foregroundColour)
{
    m_core = QCoreApplication::arguments().contains(QStringLiteral("--coreprofile"));
    pDoc = doc;


    loadDrawingGLObjects();

    loadAxes();

    if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWAXIAL) {
        loadAxialGLObjects();
    }

    if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWVGA) {
        loadVGAGLObjects();
    }

    if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWDATA) {
        loadDataMapGLObjects();
    }

    matchViewToCurrentMetaGraph();
}

GLView::~GLView()
{
    makeCurrent();
    m_axes.cleanup();
    m_grid.cleanup();
    m_visibleDrawingLines.cleanup();
    m_visiblePointMap.cleanup();
    m_visibleShapeGraph.cleanup();
    m_visibleShapeGraphPolygons.cleanup();
    m_visibleShapeGraphLinksFills.cleanup();
    m_visibleShapeGraphLinksLines.cleanup();
    m_visibleShapeGraphUnlinksFills.cleanup();
    m_visibleShapeGraphUnlinksLines.cleanup();
    m_visibleDataMapLines.cleanup();
    m_visibleDataMapPolygons.cleanup();
    doneCurrent();
}

QSize GLView::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLView::sizeHint() const
{
    return QSize(400, 400);
}

void GLView::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(qRed(m_background)/255.0f, qGreen(m_background)/255.0f, qBlue(m_background)/255.0f, 1);

    m_axes.initializeGL(m_core);
    m_visibleDrawingLines.initializeGL(m_core);
    m_visiblePointMap.initializeGL(m_core);
    m_grid.initializeGL(m_core);
    m_visibleShapeGraph.initializeGL(m_core);
    m_visibleShapeGraphPolygons.initializeGL(m_core);
    m_visibleShapeGraphLinksFills.initializeGL(m_core);
    m_visibleShapeGraphLinksLines.initializeGL(m_core);
    m_visibleShapeGraphUnlinksFills.initializeGL(m_core);
    m_visibleShapeGraphUnlinksLines.initializeGL(m_core);
    m_visibleDataMapLines.initializeGL(m_core);
    m_visibleDataMapPolygons.initializeGL(m_core);
    m_visiblePointMapLinksLines.initializeGL(m_core);
    m_visiblePointMapLinksFills.initializeGL(m_core);

    if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWVGA) {
        loadVGAGLObjectsRequiringGLContext();
    }

    m_mModel.setToIdentity();

    m_mView.setToIdentity();
    m_mView.translate(0, 0, -1);
}

void GLView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    if(datasetChanged) {

        loadDrawingGLObjects();
        m_visibleDrawingLines.updateGL(m_core);

        if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWAXIAL) {
            loadAxialGLObjects();
            m_visibleShapeGraph.updateGL(m_core);
            m_visibleShapeGraphPolygons.updateGL(m_core);
            m_visibleShapeGraphLinksFills.updateGL(m_core);
            m_visibleShapeGraphLinksLines.updateGL(m_core);
            m_visibleShapeGraphUnlinksFills.updateGL(m_core);
            m_visibleShapeGraphUnlinksLines.updateGL(m_core);
        }

        if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWDATA) {
            loadDataMapGLObjects();
            m_visibleDataMapLines.updateGL(m_core);
            m_visibleDataMapPolygons.updateGL(m_core);
        }

        if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWVGA) {
            loadVGAGLObjects();
            m_visiblePointMap.updateGL(m_core);
            m_grid.updateGL(m_core);
            m_visiblePointMapLinksLines.updateGL(m_core);
            m_visiblePointMapLinksFills.updateGL(m_core);
            loadVGAGLObjectsRequiringGLContext();
        }

        datasetChanged = false;
    }

    if(m_showLinks) {
        glLineWidth(4);
        if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWVGA) {
            m_visiblePointMapLinksFills.paintGL(m_mProj, m_mView, m_mModel);
            m_visiblePointMapLinksLines.paintGL(m_mProj, m_mView, m_mModel);
        }
        if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWAXIAL) {
            m_visibleShapeGraphLinksFills.paintGL(m_mProj, m_mView, m_mModel);
            m_visibleShapeGraphLinksLines.paintGL(m_mProj, m_mView, m_mModel);
            m_visibleShapeGraphUnlinksFills.paintGL(m_mProj, m_mView, m_mModel);
            m_visibleShapeGraphUnlinksLines.paintGL(m_mProj, m_mView, m_mModel);
        }
        glLineWidth(1);
    }
    m_visibleDrawingLines.paintGL(m_mProj, m_mView, m_mModel);

    if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWVGA) {
        if(pDoc->m_meta_graph->m_showgrid) {
            m_grid.paintGL(m_mProj, m_mView, m_mModel);
        }
        m_visiblePointMap.paintGL(m_mProj, m_mView, m_mModel);
    }

    if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWAXIAL) {
        m_visibleShapeGraph.paintGL(m_mProj, m_mView, m_mModel);
        m_visibleShapeGraphPolygons.paintGL(m_mProj, m_mView, m_mModel);
    }

    if(pDoc->m_meta_graph->getViewClass() & pDoc->m_meta_graph->VIEWDATA) {
        m_visibleDataMapLines.paintGL(m_mProj, m_mView, m_mModel);
        m_visibleDataMapPolygons.paintGL(m_mProj, m_mView, m_mModel);
    }

    m_axes.paintGL(m_mProj, m_mView, m_mModel);
}

void GLView::loadAxes() {
    std::vector<std::pair<SimpleLine, PafColor>> axesData;
    axesData.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(0,0,1,0), PafColor(1,0,0)));
    axesData.push_back(std::pair<SimpleLine, PafColor> (SimpleLine(0,0,0,1), PafColor(0,1,0)));
    m_axes.loadLineData(axesData);
}

void GLView::loadDrawingGLObjects() {
    pDoc->m_meta_graph->setLock(this);
    m_visibleDrawingLines.loadLineData(pDoc->m_meta_graph->getVisibleDrawingLines(), m_foreground);
    pDoc->m_meta_graph->releaseLock(this);
}

void GLView::loadDataMapGLObjects() {
    ShapeMap & currentDataMap = pDoc->m_meta_graph->getDisplayedDataMap();
    m_visibleDataMapLines.loadLineData(currentDataMap.getAllLinesWithColour());
    m_visibleDataMapPolygons.loadPolygonData(currentDataMap.getAllPolygonsWithColour());
}

void GLView::loadAxialGLObjects() {
    ShapeGraph &currentShapeGraph = pDoc->m_meta_graph->getDisplayedShapeGraph();
    m_visibleShapeGraph.loadLineData(currentShapeGraph.getAllLinesWithColour());
    m_visibleShapeGraphPolygons.loadPolygonData(currentShapeGraph.getAllPolygonsWithColour());

    const std::vector<SimpleLine> &linkLines = currentShapeGraph.getAllLinkLines();
    std::vector<Point2f> linkPointLocations;
    std::vector<SimpleLine>::const_iterator iter = linkLines.begin(), end =
    linkLines.end();
    for ( ; iter != end; ++iter )
    {
        SimpleLine linkLine = *iter;
        linkPointLocations.push_back(linkLine.start());
        linkPointLocations.push_back(linkLine.end());
    }

    const std::vector<Point2f> &linkFillTriangles =
            GeometryGenerators::generateMultipleDiskTriangles(32, currentShapeGraph.getSpacing()*0.1, linkPointLocations);
    m_visibleShapeGraphLinksFills.loadTriangleData(linkFillTriangles, PafColor(0,0,0));

    std::vector<SimpleLine> linkFillPerimeters =
            GeometryGenerators::generateMultipleCircleLines(32, currentShapeGraph.getSpacing()*0.1, linkPointLocations);
    linkFillPerimeters.insert( linkFillPerimeters.end(), linkLines.begin(), linkLines.end() );
    m_visibleShapeGraphLinksLines.loadLineData(linkFillPerimeters, qRgb(0,255,0));


    const std::vector<Point2f> &unlinkPoints = currentShapeGraph.getAllUnlinkPoints();

    const std::vector<Point2f> &unlinkFillTriangles =
            GeometryGenerators::generateMultipleDiskTriangles(32, currentShapeGraph.getSpacing()*0.1, unlinkPoints);
    m_visibleShapeGraphUnlinksFills.loadTriangleData(unlinkFillTriangles, PafColor(1, 1, 1));

    const std::vector<SimpleLine> &unlinkFillPerimeters =
            GeometryGenerators::generateMultipleCircleLines(32, currentShapeGraph.getSpacing()*0.1, unlinkPoints);
    m_visibleShapeGraphUnlinksLines.loadLineData(unlinkFillPerimeters, qRgb(255,0,0));
}

void GLView::loadVGAGLObjects() {
    PointMap& currentPointMap = pDoc->m_meta_graph->getDisplayedPointMap();
    QtRegion region = currentPointMap.getRegion();
    m_visiblePointMap.loadRegionData(region.bottom_left.x, region.bottom_left.y, region.top_right.x, region.top_right.y);

    if(pDoc->m_meta_graph->m_showgrid) {
        std::vector<SimpleLine> gridData;
        double spacing = currentPointMap.getSpacing();
        QRgb gridColour = colorMerge(m_foreground, m_background);
        double offsetX = region.bottom_left.x;
        double offsetY = region.bottom_left.y;
        for(int x = 1; x < currentPointMap.getCols(); x++) {
            gridData.push_back(SimpleLine(offsetX + x*spacing, region.bottom_left.y, offsetX + x*spacing, region.top_right.y));
        }
        for(int y = 1; y < currentPointMap.getRows(); y++) {
            gridData.push_back(SimpleLine(region.bottom_left.x, offsetY + y*spacing, region.top_right.x, offsetY + y*spacing));
        }
        m_grid.loadLineData(gridData, gridColour);
    }
    if(m_showLinks) {

        const std::vector<SimpleLine> &mergedPixelLines = depthmapX::getMergedPixelsAsLines(currentPointMap);
        std::vector<Point2f> mergedPixelLocations;
        std::vector<SimpleLine>::const_iterator iter = mergedPixelLines.begin(), end =
        mergedPixelLines.end();
        for ( ; iter != end; ++iter )
        {
            SimpleLine mergeLine = *iter;
            mergedPixelLocations.push_back(mergeLine.start());
            mergedPixelLocations.push_back(mergeLine.end());
        }

        const std::vector<Point2f> &linkFillTriangles =
                GeometryGenerators::generateMultipleDiskTriangles(32, currentPointMap.getSpacing()*0.25, mergedPixelLocations);
        m_visiblePointMapLinksFills.loadTriangleData(linkFillTriangles, PafColor(0,0,0));

        std::vector<SimpleLine> linkFillPerimeters =
                GeometryGenerators::generateMultipleCircleLines(32, currentPointMap.getSpacing()*0.25, mergedPixelLocations);
        linkFillPerimeters.insert( linkFillPerimeters.end(), mergedPixelLines.begin(), mergedPixelLines.end() );
        m_visiblePointMapLinksLines.loadLineData(linkFillPerimeters, qRgb(0,255,0));
    }
}
void GLView::loadVGAGLObjectsRequiringGLContext() {
    PointMap& currentPointMap = pDoc->m_meta_graph->getDisplayedPointMap();
    QImage data(currentPointMap.getCols(),currentPointMap.getRows(), QImage::Format_RGBA8888);
    data.fill(Qt::transparent);

    for (int y = 0; y < currentPointMap.getRows(); y++) {
        for (int x = 0; x < currentPointMap.getCols(); x++) {
            PixelRef pix(x, y);
            PafColor colour = currentPointMap.getPointColor( pix );
            if (colour.alphab() != 0)
            { // alpha == 0 is transparent
                data.setPixelColor(x, y, qRgb(colour.redb(),colour.greenb(),colour.blueb()));
            }
        }
    }
    m_visiblePointMap.loadPixelData(data);
}

void GLView::resizeGL(int w, int h)
{
    screenWidth = w;
    screenHeight = h;
    screenRatio = GLfloat(w) / h;
    recalcView();
}

void GLView::mouseReleaseEvent(QMouseEvent *event)
{
    Point2f worldPoint = getWorldPoint(event->pos());
    if (!pDoc->m_communicator) {
       QtRegion r( worldPoint, worldPoint );
          pDoc->m_meta_graph->setCurSel( r, false ); // <- reset current sel
       pDoc->SetRedrawFlag(QGraphDoc::VIEW_ALL,QGraphDoc::REDRAW_POINTS, QGraphDoc::NEW_SELECTION);
    }
}

void GLView::mousePressEvent(QMouseEvent *event)
{
    m_mouseLastPos = event->pos();
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_mouseLastPos.x();
    int dy = event->y() - m_mouseLastPos.y();

    if (event->buttons() & Qt::RightButton) {
        panBy(dx, dy);
    }
    m_mouseLastPos = event->pos();
}

void GLView::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;

    int x = event->x();
    int y = event->y();

    zoomBy(1 - 0.25f * numDegrees.y() / 15.0f, x, y);

    event->accept();
}

void GLView::zoomBy(float dzf, int mouseX, int mouseY)
{
    float pzf = zoomFactor;
    zoomFactor = zoomFactor * dzf;
    if(zoomFactor < minZoomFactor) zoomFactor = minZoomFactor;
    else if(zoomFactor > maxZoomFactor) zoomFactor = maxZoomFactor;
    m_eyePosX += (zoomFactor - pzf) * screenRatio * GLfloat(mouseX - screenWidth*0.5f) / GLfloat(screenWidth);
    m_eyePosY -= (zoomFactor - pzf) * GLfloat(mouseY - screenHeight*0.5f) / GLfloat(screenHeight);
    recalcView();
}
void GLView::panBy(int dx, int dy)
{
    m_eyePosX += zoomFactor * GLfloat(dx) / screenHeight;
    m_eyePosY -= zoomFactor * GLfloat(dy) / screenHeight;

    recalcView();
}
void GLView::recalcView()
{
    m_mProj.setToIdentity();

    if(perspectiveView)
    {
        m_mProj.perspective(45.0f, screenRatio, 0.01f, 100.0f);
        m_mProj.scale(1.0f, 1.0f, zoomFactor);
    }
    else
    {
        m_mProj.ortho(-zoomFactor * 0.5f * screenRatio, zoomFactor * 0.5f * screenRatio, -zoomFactor * 0.5f, zoomFactor * 0.5f, 0, 10);
    }
    m_mProj.translate(m_eyePosX, m_eyePosY, 0.0f);
    update();
}

Point2f GLView::getWorldPoint(const QPoint &screenPoint) {
    return Point2f(+ zoomFactor * float(screenPoint.x() - screenWidth*0.5)  / screenHeight - m_eyePosX,
                   - zoomFactor * float(screenPoint.y() - screenHeight*0.5) / screenHeight - m_eyePosY);

}

void GLView::matchViewToCurrentMetaGraph() {
    const QtRegion &region = pDoc->m_meta_graph->getBoundingBox();
    matchViewToRegion(region);
    recalcView();
}

void GLView::matchViewToRegion(QtRegion region) {
    if((region.top_right.x == 0 && region.bottom_left.x == 0)
            || (region.top_right.y == 0 && region.bottom_left.y == 0))
        // region is unset, don't try to change the view to it
        return;
    m_eyePosX = - (region.top_right.x + region.bottom_left.x)*0.5f;
    m_eyePosY = - (region.top_right.y + region.bottom_left.y)*0.5f;
    if(region.width() > region.height())
    {
        zoomFactor = region.top_right.x - region.bottom_left.x;
    }
    else
    {
        zoomFactor = region.top_right.y - region.bottom_left.y;
    }
    minZoomFactor = zoomFactor * 0.001;
    maxZoomFactor = zoomFactor * 10;
}

void GLView::OnModeJoin()
{
   if (pDoc->m_meta_graph->getState() & (MetaGraph::POINTMAPS | MetaGraph::SHAPEGRAPHS)) {
      m_showLinks = true;
      notifyDatasetChanged();
   }
}

void GLView::OnModeUnjoin()
{
   if (pDoc->m_meta_graph->getState() & (MetaGraph::POINTMAPS | MetaGraph::SHAPEGRAPHS)) {
      m_showLinks = true;
      notifyDatasetChanged();
   }
}
