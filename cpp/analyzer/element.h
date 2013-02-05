#ifndef ELEMENT_H
#define ELEMENT_H

#include <assert.h>

class Element {

  public:
    enum ElementType {
      INSERT,
      REMOVE,
    };

    Element(){}

    void initialize (int order, ElementType type, int value) {
      order_ = order;
      type_ = type;
      value_ = value;
    }

    void set_imaginary_value(int value) {
      assert (value_ < 0);
      value_ = value;
    }

    int order() const {
      return order_;
    }

    ElementType type() const {
      return type_;
    }

    int value() const {
      return value_;
    }

    bool is_null_return() const {
      return value_ < 0;
    }

  private: 
    int order_;
    ElementType type_;
    int value_;
};

#endif // ELEMENT_H
