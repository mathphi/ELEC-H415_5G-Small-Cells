#ifndef WALL_H
#define WALL_H

#include <QLineF>

// Wall class
class Wall
{
public:
    Wall(QLineF line);

    QLineF getLine();
    void setLine(QLineF line);

    QLineF getRealLine() const;
    double getNormalAngleTo(QLineF line) const;

    double getPermitivity() const;

private:
    QLineF m_line;
    double m_permitivity;
};

#endif // WALL_H
