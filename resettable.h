#ifndef RESETTABLE_H
#define RESETTABLE_H

/**
 * @brief The Resettable class
 *
 * Classes inherit from this interface should can be reset
 */
class Resettable {
public:
    virtual ~Resettable() {}

    virtual void reset() = 0;
};

#endif // RESETTABLE_H
