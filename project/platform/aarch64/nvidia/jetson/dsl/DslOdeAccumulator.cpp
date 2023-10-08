#include "Dsl.h"
#include "DslOdeAccumulator.h"
#include "DslOdeAction.h"

namespace DSL
{

    OdeAccumulator::OdeAccumulator(const char* name)
        : OdeBase(name)
    {
        LOG_FUNC();
    }

    OdeAccumulator::~OdeAccumulator()
    {
        LOG_FUNC();
        
        RemoveAllActions();
    }

    void OdeAccumulator::HandleOccurrences(DSL_BASE_PTR pOdeTrigger, 
        GstBuffer* pBuffer, std::vector<NvDsDisplayMeta*>& displayMetaData,
        NvDsFrameMeta* pFrameMeta)
    {
        for (const auto &imap: m_pOdeActionsIndexed)
        {
            DSL_ODE_ACTION_PTR pOdeAction = 
                std::dynamic_pointer_cast<OdeAction>(imap.second);
            try
            {
                pOdeAction->HandleOccurrence(pOdeTrigger, pBuffer, 
                    displayMetaData, pFrameMeta, NULL);
            }
            catch(...)
            {
                LOG_ERROR("ODE Accumulater '" << GetName() << "' => Action '" 
                    << pOdeAction->GetName() << "' threw exception");
            }
        }
    }

    bool OdeAccumulator::AddAction(DSL_BASE_PTR pChild)
    {
        LOG_FUNC();
        
        if (m_pOdeActions.find(pChild->GetName()) != m_pOdeActions.end())
        {
            LOG_ERROR("ODE Action '" << pChild->GetName() 
                << "' is already a child of OdeAccumulator '" << GetName() << "'");
            return false;
        }
        
        // increment next index, assign to the Action, and update parent releationship.
        pChild->SetIndex(++m_nextActionIndex);
        pChild->AssignParentName(GetName());

        // Add the shared pointer to child to both Maps, by name and index
        m_pOdeActions[pChild->GetName()] = pChild;
        m_pOdeActionsIndexed[m_nextActionIndex] = pChild;
        
        return true;
    }

    bool OdeAccumulator::RemoveAction(DSL_BASE_PTR pChild)
    {
        LOG_FUNC();
        
        if (m_pOdeActions.find(pChild->GetName()) == m_pOdeActions.end())
        {
            LOG_WARN("'" << pChild->GetName() 
                <<"' is not a child of OdeAccumulator '" << GetName() << "'");
            return false;
        }
        
        // Erase the child from both maps
        m_pOdeActions.erase(pChild->GetName());
        m_pOdeActionsIndexed.erase(pChild->GetIndex());

        // Clear the parent relationship and index
        pChild->ClearParentName();
        pChild->SetIndex(0);
        return true;
    }
    
    void OdeAccumulator::RemoveAllActions()
    {
        LOG_FUNC();
        
        for (auto &imap: m_pOdeActions)
        {
            LOG_DEBUG("Removing Action '" << imap.second->GetName() 
                <<"' from OdeAccumulator '" << GetName() << "'");
            imap.second->ClearParentName();
        }
        m_pOdeActions.clear();
        m_pOdeActionsIndexed.clear();
    }
    
}
