#include <iostream>
#include <functional>
#include <cmath>
#include <fstream>
#include <vector>
#include <algorithm>
#include <execution>

struct Point
{
    double x, y;

    Point(double const &_x, double const &_y) : x(_x), y(_y) {}

    inline Point& operator+=(Point const &b) { this->x += b.x; this->y += b.y; return *this; }
    inline Point& operator-=(Point const &b) { this->x -= b.x; this->y -= b.y; return *this; }
    inline Point& operator*=(double const &s) { this->x *= s; this->y *= s; return *this; }
    inline Point& operator/=(double const &s) { return *this *= 1/s; }
};

Point operator+(Point const &a, Point const &b) {  return {a.x+b.x, a.y+b.y}; }
Point operator-(Point const &a, Point const &b) {  return {a.x-b.x, a.y-b.y}; }
Point operator*(Point const &a, double const &s) { return {a.x * s, a.y * s}; }
Point operator*(double const &s, Point const &a) { return a * s; }
Point operator/(Point const &a, double const &s) { return a * (1/s); }

struct Rect
{
    Point tl;
    Point br;

    Rect(Point const &topLeft, Point const &bottomRight) : tl(topLeft), br(bottomRight) {}

    double width() const { return br.x - tl.x; }
    double height() const { return br.y - tl.y; }
    Point center() const { return (tl - br) / 2; }
};

struct Circle
{
    Point center;
    double radius;

    Circle(double const &x, double const &y, double const &r) : center(x,y), radius(r) {}
    Circle(Point const &c, double const &r) : Circle(c.x, c.y, r) {}

    bool contains(Point const &p) const
    {
        double dx = center.x - p.x;
        double dy = center.y - p.y;
        return (dx*dx + dy*dy < radius*radius);
    }
};

Rect outerSquare(Circle const &circle)
{
    Point r(circle.radius, circle.radius);
    return Rect(circle.center - r, circle.center + r);
}

Rect innterSquare(Circle const &circle)
{
    double const diag = std::sqrt(2) * circle.radius;
    Point r(diag, diag);
    return Rect(circle.center - r, circle.center + r);
}

Circle outerCircle(Rect const &rect) 
{ 
    double w2 = rect.width() / 2; 
    double h2 = rect.height() / 2;
    return Circle(rect.center(), std::sqrt(w2*w2+h2*h2));
}

Circle innerCircle(Rect const &rect) 
{ 
    double w2 = rect.width() / 2; 
    double h2 = rect.height() / 2;
    return Circle(rect.center(), std::min(w2, h2));
}

template<typename DataType>
class PointMap
{
public:
    PointMap(Rect const &r, uint64_t const &xp, uint64_t const &yp) : 
    rect(r), spacingX(r.width() / (double)xp), spacingY(r.height() / (double)yp)
    {
        cols.reserve(xp);
        for (uint64_t i=0; i<xp; ++i) cols.push_back(i);
        cols.shrink_to_fit();
        rows.reserve(yp);
        for (uint64_t i=0; i<yp; ++i) rows.push_back(i);
        rows.shrink_to_fit();

        data = new DataType[rows.size() * cols.size()];
    }

    PointMap(Point const &tl, Point const &br, uint64_t const &xp, uint64_t const &yp) : PointMap(Rect(tl, br), xp, yp) {}
    PointMap(Point const &tl, Point const &br, uint64_t const &p) : PointMap(Rect(tl, br), p, p) {}
    PointMap(Rect const &r, uint64_t const &p) : PointMap(r, p, p) {}

    ~PointMap()
    {
        delete[] data;
    }

    void accumulate(std::function<DataType(double const &x, double const &y)> const &f)
    {
//        for(uint64_t i=0; i<xPoints; ++i)
//        {
//            double x = rect.tl.x + i * spacingX;
//            for (uint64_t j=0; j<yPoints; ++j)
//            {
//                double y = rect.tl.y + j * spacingY;
//                data[i*yPoints + j] += f(x, y);
//            }
//        }

        std::for_each(std::execution::par_unseq, rows.begin(), rows.end(),
        [&f, this](uint64_t i)
        {
            std::for_each(cols.begin(), cols.end(),
            [&f, &i, this](uint64_t j)
            {
                double x = rect.tl.x + i * spacingX;
                double y = rect.tl.y + j * spacingY;
                data[i*cols.size() + j] += f(x, y);
            });
        });
    }

    void write()
    {
        for(uint64_t i=0; i<cols.size(); ++i)
        {
            double x = rect.tl.x + i * spacingX;
            for (uint64_t j=0; j<rows.size(); ++j)
            {
                double y = rect.tl.y + j * spacingY;
                std::cout << x << " " << y << " " << data[i*rows.size() + j] << "\n";
            }
        }
        std::cout << std::endl;
    }

private:
    PointMap() = delete;
    PointMap(PointMap const &other) = delete;
    PointMap(PointMap &&other) = delete;

    Rect const rect;
    double const spacingX;
    double const spacingY;

    std::vector<uint64_t> rows;
    std::vector<uint64_t> cols;
    DataType *data;
};

class Beam
{
public:
    virtual double intensity(double const &x, double const &y) const = 0; 
    virtual void setPosition(double const &x, double const &y) = 0;
    virtual void changeIntensity(double const &dI) = 0;
};

class CircleBeam : public Beam
{
public:
    CircleBeam(Point const &c, double const &r, double const &I) : circle(c, r), baseIntensity(I) {};
    CircleBeam(Circle const &c, double const &I) : CircleBeam(c.center, c.radius, I) {};

    virtual double intensity(double const &x, double const &y) const override
    {
        return circle.contains({x, y}) ? baseIntensity : 0;
    }

    virtual void setPosition(double const &x, double const &y)
    {
        circle.center = {x, y};
    }

    virtual void changeIntensity(double const &dI) 
    {
        baseIntensity += dI;
    }

private:
    Circle circle;
    double baseIntensity;
};

class GaussBeam : public Beam
{
public:
    GaussBeam(Point const &c, double const &r, double const &I) : center(c), radius(r), baseIntensity(I) {};

    virtual double intensity(double const &x, double const &y) const override
    {
        double dx = x - center.x;
        double dy = y - center.y;
        return baseIntensity / (radius * std::sqrt(2 * M_PI)) * std::exp(-(dx*dx+dy*dy)/(2*(radius*radius)));
    }

    virtual void setPosition(double const &x, double const &y)
    {
        center = {x, y};
    }

    virtual void changeIntensity(double const &dI) 
    {
        baseIntensity += dI;
    }

private:
    Point center;
    double radius;
    double baseIntensity;
};

int main(int argc, char **argv)
{
    std::cout << argc << "\n";
    if (argc < 5) 
    {
        std::cout << "\
Supply at least 4 parameters\n\
  1: start height (cm)\n\
  2: timestep (s)\n\
  3: end time (s)\n\
  4: beam type (c = circular, g = gaussian)\n\n\
You may supply up to 7 parameters (std is)\n\
  5: grid raduis (0.15 cm)\n\
  6: beam radius (1 cm)\n\
  7: beam intensity (2 W/cm^2)\n\
  8: grid resolution (50)\n";
        return 1;
    }

    //Have to be set
    double const g = 9.81e2;                //cm/s^2
    double startHeight = atof(argv[1]);     //cm
    double dt = atof(argv[2]);              //s
    double endTime = atof(argv[3]);         //s
    char beamType = std::tolower(argv[4][0]);

    //May be set
    double gridRadius = 0.15;               //cm
    double beamRadius = 1;                  //cm
    double intensity = 2;                   //W/cm^2
    int res = 50;
    if (argc > 5) gridRadius = atof(argv[5]);  //cm
    if (argc > 6) beamRadius = atof(argv[6]);  //cm
    if (argc > 7) intensity = atof(argv[7]);
    if (argc > 8) res = atoi(argv[8]);

    //Create the appropriate beam
    Beam *beam;
    switch (beamType)
    {
        case 'c': beam = new CircleBeam({0, 0}, beamRadius, intensity); break;
        case 'g': beam = new GaussBeam({0, 0}, beamRadius, intensity); break;
        default: std::cout << "Beam type must be c/g\n"; return 1;
    }

    //Cgreate the grid
    Circle grid(0, 0, gridRadius);
    Rect mapArea(outerSquare(grid));
    PointMap<double> map(mapArea, 50, 50);

    //Variables fot the physics simulation
    double time = 0;
    double beamVelocity = 0;
    double beamPosition = startHeight;

    while (time < endTime)
    {
        //simulate the fall
        beamVelocity -= g * dt;
        beamPosition += beamVelocity * dt;

        //advance time
        //std::cout << time << "\n";
        time += dt;

        beam->setPosition(0, beamPosition);

        //sample the intensities
        map.accumulate(
        [&grid, &beam, &dt](double const &x, double const &y) -> double
        {
            if (grid.contains({x, y}))
                return beam->intensity(x, y) * dt;
            else 
                return 0;     
        });
    }

    map.write();
}