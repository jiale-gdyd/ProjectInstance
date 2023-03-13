#pragma once

#include <map>
#include <string>
#include <iostream>

#include "log.hpp"

namespace axpi {
typedef void *(*create_function)();

extern std::map<std::string, int> &getModelTypeTable();

class ObjectFactory {
public:
    ~ObjectFactory() {

    }

    void *getObjectByName(std::string name) {
        std::map<std::string, create_function>::iterator it = mNameMap.find(name);
        if (it == mNameMap.end()) {
            return NULL;
        }

        create_function fun = it->second;
        if (!fun) {
            return NULL;
        }

        void *obj = fun();
        return obj;
    }

    void *getObjectByID(int clsid) {
        std::map<int, create_function>::iterator it = mIdMap.find(clsid);
        if (it == mIdMap.end()) {
            return NULL;
        }

        create_function fun = it->second;
        if (!fun) {
            return NULL;
        }

        void *obj = fun();
        return obj;
    }

    bool contain(std::string name) {
        std::map<std::string, create_function>::iterator it = mNameMap.find(name);
        if (it == mNameMap.end()) {
            return false;
        }

        return true;
    }

    bool contain(int clsid) {
        std::map<int, create_function>::iterator it = mIdMap.find(clsid);
        if (it == mIdMap.end()) {
            return false;
        }

        return true;
    }

    void print_obj() {
        auto it = mNameMap.begin();
        auto itid = mIdMap.begin();

        for (size_t i = 0; i < mNameMap.size(); i++) {
            axmpi_info("%s-%d", it->first.c_str(), itid->first);
            it++;
            itid++;
        }
    }

    void registClass(int clsid, std::string name, create_function fun) {
        mIdMap[clsid] = fun;
        mNameMap[name] = fun;
        getModelTypeTable()[name] = clsid;
    }

    static ObjectFactory &getInstance() {
        static ObjectFactory fac;
        return fac;
    }

private:
    ObjectFactory() {}

    std::map<int, create_function>         mIdMap;
    std::map<std::string, create_function> mNameMap;
};

class RegisterAction {
public:
    RegisterAction(int clsid, std::string className, create_function ptrCreateFn) {
        ObjectFactory::getInstance().registClass(clsid, className, ptrCreateFn);
    }
};

#define REGISTER(clsid, className)                       \
    static void *objectCreator_##className()             \
    {                                                    \
        return new className;                            \
    }                                                    \
                                                         \
    static RegisterAction g_creatorRegister_##className(clsid, #clsid, (create_function)objectCreator_##className);
}
