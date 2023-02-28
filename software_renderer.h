#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


    // Implements SoftwareRenderer //

    void SoftwareRendererImp::draw_svg(SVG& svg) {

        // set top level transformation
        transformation = svg_2_screen;

        // draw all elements
        for (size_t i = 0; i < svg.elements.size(); ++i) {
            draw_element(svg.elements[i]);
        }

        // draw canvas outline
        Vector2D a = transform(Vector2D(0, 0)); a.x--; a.y--;
        Vector2D b = transform(Vector2D(svg.width, 0)); b.x++; b.y--;
        Vector2D c = transform(Vector2D(0, svg.height)); c.x--; c.y++;
        Vector2D d = transform(Vector2D(svg.width, svg.height)); d.x++; d.y++;

        rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
        rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
        rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
        rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

        // resolve and send to render target
        resolve();

    }

    void SoftwareRendererImp::set_sample_rate(size_t sample_rate) {


        this->sample_rate = sample_rate;

    }

    void SoftwareRendererImp::set_render_target(unsigned char* render_target,
        size_t width, size_t height) {


        this->render_target = render_target;
        this->target_w = width;
        this->target_h = height;

    }

    void SoftwareRendererImp::draw_element(SVGElement* element) {


        switch (element->type) {
        case POINT:
            draw_point(static_cast<Point&>(*element));
            break;
        case LINE:
            draw_line(static_cast<Line&>(*element));
            break;
        case POLYLINE:
            draw_polyline(static_cast<Polyline&>(*element));
            break;
        case RECT:
            draw_rect(static_cast<Rect&>(*element));
            break;
        case POLYGON:
            draw_polygon(static_cast<Polygon&>(*element));
            break;
        case ELLIPSE:
            draw_ellipse(static_cast<Ellipse&>(*element));
            break;
        case IMAGE:
            draw_image(static_cast<Image&>(*element));
            break;
        case GROUP:
            draw_group(static_cast<Group&>(*element));
            break;
        default:
            break;
        }

    }


    // Primitive Drawing //

    void SoftwareRendererImp::draw_point(Point& point) {

        Vector2D p = transform(point.position);
        rasterize_point(p.x, p.y, point.style.fillColor);

    }

    void SoftwareRendererImp::draw_line(Line& line) {

        Vector2D p0 = transform(line.from);
        Vector2D p1 = transform(line.to);
        rasterize_line(p0.x, p0.y, p1.x, p1.y, line.style.strokeColor);

    }
    void SoftwareRendererImp::draw_polyline(Polyline& polyline) {

        Color c = polyline.style.strokeColor;

        if (c.a != 0) {
            int nPoints = polyline.points.size();
            for (int i = 0; i < nPoints - 1; i++) {
                Vector2D p0 = transform(polyline.points[(i + 0) % nPoints]);
                Vector2D p1 = transform(polyline.points[(i + 1) % nPoints]);
                rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
            }
        }
    }

    void SoftwareRendererImp::draw_rect(Rect& rect) {

        Color c;

        // draw as two triangles
        float x = rect.position.x;
        float y = rect.position.y;
        float w = rect.dimension.x;
        float h = rect.dimension.y;

        Vector2D p0 = transform(Vector2D(x, y));
        Vector2D p1 = transform(Vector2D(x + w, y));
        Vector2D p2 = transform(Vector2D(x, y + h));
        Vector2D p3 = transform(Vector2D(x + w, y + h));

        // draw fill
        c = rect.style.fillColor;
        if (c.a != 0) {
            rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
            rasterize_triangle(p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c);
        }

        // draw outline
        c = rect.style.strokeColor;
        if (c.a != 0) {
            rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
            rasterize_line(p1.x, p1.y, p3.x, p3.y, c);
            rasterize_line(p3.x, p3.y, p2.x, p2.y, c);
            rasterize_line(p2.x, p2.y, p0.x, p0.y, c);
        }

    }

    void SoftwareRendererImp::draw_polygon(Polygon& polygon) {

        Color c;

        // draw fill
        c = polygon.style.fillColor;
        if (c.a != 0) {

            // triangulate
            vector<Vector2D> triangles;
            triangulate(polygon, triangles);

            // draw as triangles
            for (size_t i = 0; i < triangles.size(); i += 3) {
                Vector2D p0 = transform(triangles[i + 0]);
                Vector2D p1 = transform(triangles[i + 1]);
                Vector2D p2 = transform(triangles[i + 2]);
                rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
            }
        }

        // draw outline
        c = polygon.style.strokeColor;
        if (c.a != 0) {
            int nPoints = polygon.points.size();
            for (int i = 0; i < nPoints; i++) {
                Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
                Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
                rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
            }
        }
    }

    void SoftwareRendererImp::draw_ellipse(Ellipse& ellipse) {


    }

    void SoftwareRendererImp::draw_image(Image& image) {

        Vector2D p0 = transform(image.position);
        Vector2D p1 = transform(image.position + image.dimension);

        rasterize_image(p0.x, p0.y, p1.x, p1.y, image.tex);
    }

    void SoftwareRendererImp::draw_group(Group& group) {

        for (size_t i = 0; i < group.elements.size(); ++i) {
            draw_element(group.elements[i]);
        }

    }

    // Rasterization //

    // The input arguments in the rasterization functions 
    // below are all defined in screen space coordinates

    void SoftwareRendererImp::rasterize_point(float x, float y, Color color) {

        // fill in the nearest pixel
        int sx = (int)floor(x);
        int sy = (int)floor(y);

        // check bounds
        if (sx < 0 || sx >= target_w) return;
        if (sy < 0 || sy >= target_h) return;

        // fill sample - NOT doing alpha blending!
        render_target[4 * (sx + sy * target_w)] = (uint8_t)(color.r * 255);
        render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(color.g * 255);
        render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(color.b * 255);
        render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(color.a * 255);

    }

    void SoftwareRendererImp::rasterize_line(float x0, float y0,
        float x1, float y1,
        Color color) {
        float dx = x1 - x0;             //sets up variables
        float dy = y1 - y0;
        float dx1 = fabs(dx);
        float dy1 = fabs(dy);
        float px = 2 * dy1 - dx1;      //sets up varibles with basic calculations
        float py = 2 * dx1 - dy1;
        float x;
        float y;
        float savex;
        float savey;
        int i;
        if (dy1 <= dx1)           //if statement to get left and right sides of the circle
        {
            if (dx >= 0)        //if statement to set variables for both sides dpenging on what side is being created
            {
                x = x0;
                y = y0;
                savex = x1;
            }
            else
            {
                x = x1;
                y = y1;
                savex = x0;
            }
            rasterize_point(x, y, color);          //sets first point 
            for (i = 0; x < savex; i++)
            {
                x = x + 1;
                if (px < 0)                       //checks other side
                {
                    px = px + 2 * dy1;          //formual for new px
                }
                else
                {
                    if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))   //checks permitors
                    {
                        y = y + 1;   //adds one to the y value if it fallows the line
                    }
                    else
                    {
                        y = y - 1;      // if not gets rid of y 
                    }
                    px = px + 2 * (dy1 - dx1);    //changes px value
                }
                rasterize_point(x, y, color);     //rasterizes points
            }
        }
        else
        {
            if (dy >= 0)     //sets up for top and bottom variables
            {
                x = x0;       //sets up x and y variable for bottom and top
                y = y0;      
                savey = y1;
            }
            else
            {
                x = x1;
                y = y1;
                savey = y0;
            }
            rasterize_point(x, y, color);      //rasterzies point
            for (i = 0; y < savey; i++)
            {
                y = y + 1;
                if (py <= 0)                //checks py value 
                {
                    py = py + 2 * dx1;      //py formula for next point
                }
                else
                {
                    if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0)) //checks that the point is in bounds
                    {
                        x = x + 1;//adss x if yes
                    }
                    else
                    {
                        x = x - 1; //gets rid of 1 from x if no
                    }
                    py = py + 2 * (dx1 - dy1);   //changes formula
                }
                rasterize_point(x, y, color);        //rasterzies points
            }
        }
    }

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
    int minX = (int)floor(min({ x0, x1, x2 }));
    int maxX = (int)floor(max({ x0, x1, x2 }));
    int minY = (int)floor(min({ y0, y1, y2 }));
    int maxY = (int)floor(max({ y0, y1, y2 }));     //finds min/max values of floor

    float dx0 = x1 - x0;
    float dy0 = y1 - y0;
    float dx1 = x2 - x1;      //variables used to find differeces
    float dy1 = y2 - y1;
    float dx2 = x0 - x2;
    float dy2 = y0 - y2;
    float SSAA = dx0 * dy1 - dy0 * dx1;   //finds variables for SSAA in order to render triangles to be 
    //tirangles and not squares by using the anti alling in order to compare each points and printing a btter render

    for (int x = minX; x <= maxX; x++) {       //the first two loops go through x and y variables
        for (int y = minY; y <= maxY; y++) {
            for (int a = 0; a < sample_rate; a++) {        //using the sample rate we can create a for loop that refines the sample points
                for (int b = 0; b < sample_rate; b++) {
                    float px = (a) , py = (b) ;
                    float p0 = (y + py - y0) * dx0 - (x + px - x0) * dy0;
                    float p1 = (y + py - y1) * dx1 - (x + px - x1) * dy1; //creates new variables to set permitters 
                    float p2 = (y + py - y2) * dx2 - (x + px - x2) * dy2;
                    if (p0 * SSAA >= 0 && p1 * SSAA >= 0 && p2 * SSAA >= 0) //makes sure it is drawn in the parmitter 
                        rasterize_point(x + px, y + py, color);  //renders code
                }
            }
        }
    }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {


  return;

}


} // namespace CMU462
