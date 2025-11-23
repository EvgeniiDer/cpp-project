
#include <iostream>
#include <vector>
#include <memory>
#include <random>
#include <cmath>
#include <string>

class Shape {
public:
    virtual ~Shape() = default;
    
    virtual double area() const = 0;
    virtual double perimeter() const = 0;
    virtual void printProperties() const = 0;
    virtual void draw() const = 0;
    virtual std::string getName() const = 0;
};


class Square : public Shape {
private:
    double side;
    
public:
    Square(double s) : side(s) {}
    
    double area() const override {
        return side * side;
    }
    
    double perimeter() const override {
        return 4 * side;
    }
    
    void printProperties() const override {
        std::cout << "=== Квадрат ===" << std::endl;
        std::cout << "Длина стороны: " << side << std::endl;
        std::cout << "Площадь: " << area() << std::endl;
        std::cout << "Периметр: " << perimeter() << std::endl;
    }
    
    void draw() const override {
        std::cout << "Рисунок квадрата:" << std::endl;
        for(int i = 0; i < 6; ++i) {
            for(int j = 0; j < 6; ++j) {
                if(i == 0 || i == 5 || j == 0 || j == 5)
                    std::cout << "* ";
                else
                    std::cout << "  ";
            }
            std::cout << std::endl;
        }
    }
    
    std::string getName() const override {
        return "Квадрат";
    }
};


class Rectangle : public Shape {
private:
    double width, height;
    
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    
    double area() const override {
        return width * height;
    }
    
    double perimeter() const override {
        return 2 * (width + height);
    }
    
    void printProperties() const override {
        std::cout << "=== Прямоугольник ===" << std::endl;
        std::cout << "Ширина: " << width << std::endl;
        std::cout << "Высота: " << height << std::endl;
        std::cout << "Площадь: " << area() << std::endl;
        std::cout << "Периметр: " << perimeter() << std::endl;
    }
    
    void draw() const override {
        std::cout << "Рисунок прямоугольника:" << std::endl;
        for(int i = 0; i < 4; ++i) {
            for(int j = 0; j < 8; ++j) {
                if(i == 0 || i == 3 || j == 0 || j == 7)
                    std::cout << "* ";
                else
                    std::cout << "  ";
            }
            std::cout << std::endl;
        }
    }
    
    std::string getName() const override {
        return "Прямоугольник";
    }
};

 
class Circle : public Shape {
private:
    double radius;
    
public:
    Circle(double r) : radius(r) {}
    
    double area() const override {
        return M_PI * radius * radius;
    }
    
    double perimeter() const override {
        return 2 * M_PI * radius;
    }
    
    void printProperties() const override {
        std::cout << "=== Круг ===" << std::endl;
        std::cout << "Радиус: " << radius << std::endl;
        std::cout << "Площадь: " << area() << std::endl;
        std::cout << "Длина окружности: " << perimeter() << std::endl;
    }
    
    void draw() const override {
        std::cout << "Рисунок круга:" << std::endl;
        std::cout << "    ****    " << std::endl;
        std::cout << "  *      *  " << std::endl;
        std::cout << " *        * " << std::endl;
        std::cout << " *        * " << std::endl;
        std::cout << "  *      *  " << std::endl;
        std::cout << "    ****    " << std::endl;
    }
    
    std::string getName() const override {
        return "Круг";
    }
};


class Triangle : public Shape {
private:
    double side;
    
public:
    Triangle(double s) : side(s) {}
    
    double area() const override {
        return (sqrt(3) / 4) * side * side;
    }
    
    double perimeter() const override {
        return 3 * side;
    }
    
    void printProperties() const override {
        std::cout << "=== Треугольник (равносторонний) ===" << std::endl;
        std::cout << "Длина стороны: " << side << std::endl;
        std::cout << "Площадь: " << area() << std::endl;
        std::cout << "Периметр: " << perimeter() << std::endl;
    }
    
    void draw() const override {
        std::cout << "Рисунок треугольника:" << std::endl;
        std::cout << "    *    " << std::endl;
        std::cout << "   * *   " << std::endl;
        std::cout << "  *   *  " << std::endl;
        std::cout << " *     * " << std::endl;
        std::cout << "*********" << std::endl;
    }
    
    std::string getName() const override {
        return "Треугольник";
    }
};

// Функция для генерации случайной фигуры
std::unique_ptr<Shape> generateRandomShape() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> typeDist(0, 3);
    static std::uniform_real_distribution<> sizeDist(1.0, 10.0);
    
    int type = typeDist(gen);
    double size1 = sizeDist(gen);
    double size2 = sizeDist(gen);
    
    switch(type) {
        case 0:
            return std::make_unique<Square>(size1);
        case 1:
            return std::make_unique<Rectangle>(size1, size2);
        case 2:
            return std::make_unique<Circle>(size1);
        case 3:
            return std::make_unique<Triangle>(size1);
        default:
            return std::make_unique<Square>(1.0);
    }
}

int main() {
    const int NUM_SHAPES = 8;
    std::vector<std::unique_ptr<Shape>> shapes;
    
    std::cout << "Генерация " << NUM_SHAPES << " случайных фигур..." << std::endl;
    std::cout << "=========================================" << std::endl;
    
    for(int i = 0; i < NUM_SHAPES; ++i) {
        shapes.push_back(generateRandomShape());
    }
    
    for(size_t i = 0; i < shapes.size(); ++i) {
        std::cout << "\nФИГУРА " << (i + 1) << ": " << shapes[i]->getName() << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
        
        shapes[i]->printProperties();
        std::cout << std::endl;
        shapes[i]->draw();
        std::cout << std::endl;
    }
    
    return 0;
}