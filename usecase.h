#ifndef USECASE_H
#define USECASE_H

#include "repository.h"

class Usecase {
    Repository* repository;

   public:
    virtual ~Usecase(){};
    Usecase(){};
    Usecase(Repository* repository) : repository(repository){};
};

#endif