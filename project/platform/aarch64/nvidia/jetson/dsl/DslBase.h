#ifndef DSL_DSL_BASE_H
#define DSL_DSL_BASE_H

#include "Dsl.h"
#include "DslApi.h"

namespace DSL
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_BASE_PTR std::shared_ptr<Base>

    /**
     * @class Base
     * @brief Base class for all DSL objects.
     */
    class Base : public std::enable_shared_from_this<Base>
    {
    public: 
    
        /**
         * @brief ctor for DSL Base class
         * @param name unique object name
         */
        Base(const char* name)
            : m_name(name)
            , m_index(0)
        {
            LOG_FUNC();
        }

        /**
         * @brief dtor for DSL Base class
         */
        ~Base()
        {
            LOG_FUNC();
        }
        
        /**
         * @brief Allows a client to determined derived type from base pointer 
         * @param[in] typeInfo to compare against
         * @return true if this DSL Object is of typeInfo, false otherwise
         */
        bool IsType(const std::type_info& typeInfo)
        {
            LOG_FUNC();
            
            return (typeInfo.hash_code() == typeid(*this).hash_code());
        }

        /**
         * @brief returns the name given to this DSL Object on creation
         * @return const std::string name given to this Event
         */
        const std::string& GetName()
        {
            LOG_FUNC();
            
            return m_name;
        }
        
        /**
         * @brief updates the current name by appending a suffix
         * @return const std::string name given to this Event
         */
        void AppendSuffix(const char* suffix)
        {
            m_name += "-";
            m_name += suffix;
        }
        
        /**
         * @brief returns the currently assigned Index used for mapping order
         * @return current index value, default = 0, updated with SetIndex()
         */
        uint GetIndex()
        {
            LOG_FUNC();
            
            return m_index;
        }
        
        /**
         * @brief sets the Index for map sorting
         */
        void SetIndex(uint index)
        {
            LOG_FUNC();
            
            m_index = index;
        }
        
        /**
         * @brief returns the name given to this DSL Object on creation
         * @return const c_str name given to this DSL Object
         */
        const char* GetCStrName()
        {
            LOG_FUNC();
            
            return m_name.c_str();
        }

        /**
         * @brief called to determine if a DSL Object is currently in use 
         * - i.e. has a Parent.
         * @return true if this Object has a Parent, false otherwise
         */
        virtual bool IsInUse()
        {
            LOG_FUNC();
            
            return (bool)m_parentName.size();
        }

        /**
         * @brief function to determine if a Nodetr is a child of this Nodetr.
         * @param[in] pChild Nodetr to test for the child relationship.
         * @return true if pChild is a child of this Nodetr.
         */
        virtual bool IsChild(DSL_BASE_PTR pChild)
        {
            LOG_FUNC();
            
            return (m_pChildren.find(pChild->GetName()) != m_pChildren.end());
        }

        /**
         * @brief determines whether this Object is a child of a given Parent Object.
         * @param[in] parentName name of the object to check for a Parental.
         * relationship.
         * @return True if the provided Name is this Object's Parent
         */
        virtual bool IsParent(DSL_BASE_PTR pParent)
        {
            LOG_FUNC();
            
            return m_parentName == pParent->GetName();
        }

        /**
         * @brief Assigns this Object's parent name. Having a Parent name
         * indicates that the Object is "In-Use"
         * @param name parent name to assigne to object
         */
        void AssignParentName(const std::string& name)
        {
            LOG_FUNC();

            m_parentName.assign(name);
        }
        
        /**
         * @brief Clears this Object's parent name indicating that the Object 
         * is no longer "In-Use"
         */
        void ClearParentName()
        {
            LOG_FUNC();
            
            m_parentName.clear();
        }
        
        /**
         * @brief adds a child Object to this parent Object
         * @param[in] pChild child Object to add. 
         */
        virtual bool AddChild(DSL_BASE_PTR pChild)
        {
            LOG_FUNC();
            
            if (IsChild(pChild))
            {
                LOG_ERROR("Object '" << pChild->GetName() 
                    << "' is already a child of Object '" << GetName() << "'");
                return false;
            }
            m_pChildren[pChild->GetName()] = pChild;
            pChild->AssignParentName(GetName());
                            
            LOG_DEBUG("Child '" << pChild->m_name 
                <<"' added to Parent '" << m_name << "'");
            
            return true;
        }
        
        /**
         * @brief removes a child Object from this parent Object
         * @param[in] pChild child Object to remove.
         */
        virtual bool RemoveChild(DSL_BASE_PTR pChild)
        {
            LOG_FUNC();
            
            if (!IsChild(pChild))
            {
                LOG_WARN("'" << pChild->m_name 
                    <<"' is not a child of Parent '" << m_name << "'");
                return false;
            }
            m_pChildren.erase(pChild->m_name);
            pChild->ClearParentName();
                            
            LOG_DEBUG("Child '" << pChild->m_name 
                <<"' removed from Parent '" << m_name << "'");
            
            return true;
        }

        /**
         * @brief removes all child Objects from this parent Object
         */
        virtual void RemoveAllChildren()
        {
            LOG_FUNC();

            for (auto &imap: m_pChildren)
            {
                LOG_DEBUG("Removing Child '" << imap.second->GetName() 
                    <<"' from Parent '" << GetName() << "'");
                imap.second->ClearParentName();
            }
            m_pChildren.clear();
        }
        
        /**
         * @brief get the current number of children for this Object 
         * @return the number of Child Object held by this Parent Object
         */
        virtual uint GetNumChildren()
        {
            LOG_FUNC();
            
            return m_pChildren.size();
        }

        
    protected:

        /**
         * @brief Unique name for this DSL object
         */
        std::string m_name;
        
        /**
         * @brief Index used when sorting derived objects of DslBase
         */
        uint m_index;

        /**
         * @brief Unique name of the Parent DSL object if in use.
         * Empty string if not-in-use
         */
        std::string m_parentName;

        /**
         * @brief map of Child objects in-use by this Object
         */
        std::map<std::string, DSL_BASE_PTR> m_pChildren;
        
        std::time_t m_timeCreated;
        std::time_t m_timeModified;
        
    };
}


#endif // _DSL_BASE_H
