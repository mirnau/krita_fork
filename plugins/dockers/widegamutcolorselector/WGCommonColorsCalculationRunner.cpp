/*
 * SPDX-FileCopyrightText: 2021 Mathias Wein <lynx.mw+kde@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "WGCommonColorsCalculationRunner.h"

/*
 * NOTE: Implementation is taken from KisCommonColorsRecalculationRunner
 * from Advanced Color Selector Docker, originally written by Adam Celarek
 *
 * It is currently limited to sRGB and is more of a placeholder
 */

#include <QImage>

#include "kis_image.h"
#include "KoColor.h"
#include "KoColorSpaceRegistry.h"

namespace {

enum ColorAxis {RedAxis=0, GreenAxis, BlueAxis};

class Color
{
public:
    explicit Color(QRgb rgb) : r(qRed(rgb)), g(qGreen(rgb)), b(qBlue(rgb)) {}
    unsigned char r;
    unsigned char g;
    unsigned char b;
    inline unsigned char operator[](ColorAxis i) const
    {
        if(i==RedAxis) return r;
        if(i==GreenAxis) return g;
        return b;
    }
};

class VBox
{
    QList<Color> m_colors;
public:
    explicit VBox(const QList<QRgb> &rgbList)
    {
        QList<Color> colorList;
        for(int i=0; i<rgbList.size(); i++) {
            colorList.append(Color(rgbList.at(i)));
        }
        m_colors = colorList;
    }

    explicit VBox(const QList<Color> &colorList) : m_colors(colorList) {}

    int population() const { return m_colors.size(); }

    VBox divide()
    {
        ColorAxis axis = biggestAxis();
        Q_ASSERT(axisSize(axis)>=3);

        unsigned char divpos = divPos(axis);
        QList<Color> newVBoxColors;
        for(int i=m_colors.size()-1; i>=0; i--) {
            Color c = m_colors.at(i);
            if(c[axis]>divpos) {
                m_colors.removeAt(i);
                newVBoxColors.append(c);
            }
        }

        return VBox(newVBoxColors);
    }

    QRgb mean() const
    {
        int r=0;
        int g=0;
        int b=0;
        for(int i=0;i<m_colors.size(); i++) {
            r+=(int) m_colors.at(i)[RedAxis];
            g+=(int) m_colors.at(i)[GreenAxis];
            b+=(int) m_colors.at(i)[BlueAxis];
        }
        int size = m_colors.size();
        Q_ASSERT(size>0);

        return qRgb(r/size, g/size, b/size);
    }

    unsigned char axisSize(ColorAxis axis) const
    {
        unsigned char valMin = 255;
        unsigned char valMax = 0;
        for(int i=0; i<m_colors.size(); i++) {
            if(m_colors.at(i)[axis]>valMax)
                valMax=m_colors.at(i)[axis];
            if(m_colors.at(i)[axis]<valMin)
                valMin=m_colors.at(i)[axis];
        }
        return valMax-valMin;
    }

    ColorAxis biggestAxis() const
    {
        unsigned char sR = axisSize(RedAxis);
        unsigned char sG = axisSize(GreenAxis);
        unsigned char sB = axisSize(BlueAxis);
        if(sR>sG && sR>sB) return RedAxis;
        if(sG>sR && sG>sB) return GreenAxis;
        return BlueAxis;
    }

private:
//    unsigned char divPos(ColorAxis axis) const
//    {
//        QList<unsigned char> values;
//        for(int i=0;i<m_colors.size(); i++) {
//            values.append(m_colors.at(i)[axis]);
//        }
//        std::sort(values.begin(), values.end());
//        return values.at(values.size()*4/5);
//    }
    unsigned char divPos(ColorAxis axis) const
    {
        short min=m_colors.at(0)[axis];
        short max=m_colors.at(0)[axis];
        for(int i=0;i<m_colors.size(); i++) {
            if(min>m_colors.at(i)[axis]) min=m_colors.at(i)[axis];
            if(max<m_colors.at(i)[axis]) max=m_colors.at(i)[axis];
        }

        return (min+max)/2;
    }
};

} // anonymous namespace

WGCommonColorsCalculationRunner::WGCommonColorsCalculationRunner(KisImageSP image, int numberOfColors, QSharedPointer<QVector<KoColor> > colorStore)
    : m_numColors(numberOfColors)
    , m_commonColors(colorStore)
{
    // TODO: can this be done in the worker thread too?
    KisPaintDeviceSP device = image->projection();
    m_imageData = device->createThumbnail(1024, 1024, image->bounds(), 1, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());

}

void WGCommonColorsCalculationRunner::run()
{
    extractColors();
    Q_EMIT sigDone();
}

void WGCommonColorsCalculationRunner::extractColors()
{
    QList<QRgb> colors = getColors();

    VBox startBox(colors);
    QList<VBox> boxes;
    boxes.append(startBox);

    while (boxes.size()<m_numColors*3/5 && colors.size()>m_numColors*3/5) {
        int biggestBox=-1;
        int biggestBoxPopulation=-1;

        for(int i=0; i<boxes.size(); i++) {
            if(boxes.at(i).population()>biggestBoxPopulation &&
               boxes.at(i).axisSize(boxes.at(i).biggestAxis())>=3) {
                biggestBox=i;
                biggestBoxPopulation=boxes.at(i).population();
            }
        }

        if(biggestBox==-1 || boxes[biggestBox].population()<=3)
            break;
        VBox newBox = boxes[biggestBox].divide();
        boxes.append(newBox);
    }


    while (boxes.size()<m_numColors && colors.size()>m_numColors) {
        int biggestBox=-1;
        int biggestBoxAxisSize=-1;

        for(int i=0; i<boxes.size(); i++) {
            if(boxes.at(i).axisSize(boxes.at(i).biggestAxis())>biggestBoxAxisSize &&
               boxes.at(i).axisSize(boxes.at(i).biggestAxis())>=3) {
                biggestBox=i;
                biggestBoxAxisSize=boxes.at(i).axisSize(boxes.at(i).biggestAxis());
            }
        }

        if(biggestBox==-1 || boxes[biggestBox].population()<=3)
            break;
        VBox newBox = boxes[biggestBox].divide();
        boxes.append(newBox);
    }

    const KoColorSpace* colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    QVector<KoColor> &colorList = *m_commonColors;

    for(int i=0; i<boxes.size(); i++) {
        if(boxes.at(i).population()>=1) {
            colorList.append(KoColor(QColor(boxes.at(i).mean()), colorSpace));
        }
    }
}

QList<QRgb> WGCommonColorsCalculationRunner::getColors()
{
    int width = m_imageData.width();
    int height = m_imageData.height();

    QImage tmpImage;
    int pixelCount = height*width;
    if(pixelCount> (1<<16)) {
        qreal factor = sqrt((1<<16)/(qreal) pixelCount);
        tmpImage = m_imageData.scaledToWidth(width*factor);
    }
    else {
        tmpImage = m_imageData;
    }
    width=tmpImage.width();
    height=tmpImage.height();

    QSet<QRgb> colorList;

    for (int i=0; i<width; i++) {
        for (int j=0; j<height; j++) {
            colorList.insert(tmpImage.pixel(i, j)|qRgba(0,0,0,255));
        }
    }
    return QList<QRgb>(colorList.begin(), colorList.end());
}
