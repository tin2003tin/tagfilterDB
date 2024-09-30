#ifndef STACKABLE_HPP
#define STACKABLE_HPP

class Stackable
{
public:
    virtual ~Stackable() = default;
};

class Goto : public Stackable
{
public:
    int state;
    Goto(int state) : state(state) {}
};

class Terminal : public Stackable
{
public:
    std::string value;
    Terminal(std::string ter) : value(ter) {}
};

#endif
