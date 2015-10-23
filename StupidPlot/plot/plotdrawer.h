#pragma once

#include <vector>
#include <windows.h>
#include <gdiplus.h>

#include <formula/expression.h>
#include <plot/plotoptions.h>
#include <plot/provider/provider.h>
#include <plot/provider/gdiprovider.h>
#include <plot/provider/gdiplusprovider.h>

using std::vector;

namespace StupidPlot
{
    namespace Plot
    {
        class PlotDrawer
        {
        protected:
            HDC                     hdc;
            PlotOptions             * options = NULL;
            Provider::Provider      * provider = NULL;

            int                     width;
            int                     height;

            inline double translateCanvasX(int x)
            {
                return (static_cast<double>(x) / width) * (options->right - options->left) + options->left;
            }

            inline double translateCanvasY(int y)
            {
                return (static_cast<double>(y) / height) * (options->top - options->bottom) + options->top;
            }

            inline double translateFormulaX(double x)
            {
                return (x - (options->left)) / (options->right - options->left) * width;
            }

            inline double translateFormulaY(double y)
            {
                return (y - (options->bottom)) / (options->top - options->bottom) * height;
            }

            shared_ptr<vector<Gdiplus::PointF>> getFormulaPoints(Expression * formula)
            {
                auto ret = shared_ptr<vector<Gdiplus::PointF>>(new vector<Gdiplus::PointF>());
                ret->reserve(width);

                for (int pos_x = 0; pos_x < width; ++pos_x)
                {
                    double x = translateCanvasX(pos_x);
                    formula->setVar(0, x);
                    double y = formula->eval();
                    double pos_y = translateFormulaY(y);
                    if (pos_y >= 0 && pos_y <= height)
                    {
                        ret->push_back(Gdiplus::PointF(
                            static_cast<float>(pos_x),
                            static_cast<float>(pos_y)
                            ));
                    }
                }

                return ret;
            }

            vector<int> getGridLines(BOOL vertical = false)
            {
                vector<int> points;
                int lastDrawScreenPos = -100;
                int min = static_cast<int>(vertical ? options->bottom : options->left);
                int max = static_cast<int>(vertical ? options->top : options->right);
                for (int p = min; p < max; ++p)
                {
                    if (p % options->gridSpacing == 0)
                    {
                        int canvasPos = static_cast<int>(vertical ? translateFormulaY(p) : translateFormulaX(p));
                        if (canvasPos - lastDrawScreenPos >= 10)
                        {
                            points.push_back(canvasPos);
                            lastDrawScreenPos = canvasPos;
                        }
                    }
                }
                return points;
            }

        public:
            PlotDrawer(PlotOptions * _options, HDC _hdc)
            {
                options = _options;
                hdc = _hdc;
                provider = new Provider::GdiPlusProvider(hdc);
            }

            ~PlotDrawer()
            {
                delete provider;
            }

            void draw(int canvasWidth, int canvasHeight)
            {
                DWORD tStart = GetTickCount();

                width = canvasWidth;
                height = canvasHeight;

                provider->beginDraw();

                // Draw grid lines
                provider->drawGridLine(false, getGridLines(false), width, height);
                provider->drawGridLine(true, getGridLines(true), width, height);

                // Draw formulas
                for (size_t i = 0; i < options->formulaObjects.size(); ++i)
                {
                    provider->drawPlotLine(
                        getFormulaPoints(options->formulaObjects[i]),
                        options->formulaColors[i]
                        );
                }

                provider->endDraw();

                DWORD tEnd = GetTickCount();
                DWORD d = tEnd - tStart;
                if (d == 0) d = 1;

                Debug() << L"Draw: " << d << L", FPS = " << (1000 / d) >> Debug::writeln;
            }
        };
    }
}