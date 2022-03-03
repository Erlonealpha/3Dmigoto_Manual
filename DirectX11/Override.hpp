#pragma once

#include <DirectXMath.h>
#include <d3d11_1.h>
#include <vector>

#include "util.h"
#include "Input.hpp"
#include "HackerDevice.hpp"

enum class KeyOverrideType {
    INVALID = -1,
    ACTIVATE,
    HOLD,
    TOGGLE,
    CYCLE,
};
static EnumName_t<const wchar_t *, KeyOverrideType> KeyOverrideTypeNames[] = {
    {L"activate", KeyOverrideType::ACTIVATE},
    {L"hold", KeyOverrideType::HOLD},
    {L"toggle", KeyOverrideType::TOGGLE},
    {L"cycle", KeyOverrideType::CYCLE},
    {NULL, KeyOverrideType::INVALID} // End of list marker
};

enum class TransitionType {
    INVALID = -1,
    LINEAR,
    COSINE,
};
static EnumName_t<const char *, TransitionType> TransitionTypeNames[] = {
    {"linear", TransitionType::LINEAR},
    {"cosine", TransitionType::COSINE},
    {NULL, TransitionType::INVALID} // End of list marker
};

struct OverrideParam
{
    int idx;
    float DirectX::XMFLOAT4::*component;

    OverrideParam(int idx, float DirectX::XMFLOAT4::*component)
    {
        this->idx = idx;
        this->component = component;
    }

    char chr() const
    {
        // Oh come on C++, a pointer to member is just an offset you
        // could test directly... Fine, let's dance:
        switch ((uintptr_t)&((DirectX::XMFLOAT4*)(NULL)->*component)) {
            case (offsetof(DirectX::XMFLOAT4, x)): return 'x';
            case (offsetof(DirectX::XMFLOAT4, y)): return 'y';
            case (offsetof(DirectX::XMFLOAT4, z)): return 'z';
            case (offsetof(DirectX::XMFLOAT4, w)): return 'w';
        }
        return '?';
    };
};
static inline bool operator<(const OverrideParam &lhs, const OverrideParam &rhs)
{
    if (lhs.idx != rhs.idx)
        return (lhs.idx < rhs.idx);
    return ((uintptr_t)&((DirectX::XMFLOAT4*)(NULL)->*(lhs.component)) <
            (uintptr_t)&((DirectX::XMFLOAT4*)(NULL)->*(rhs.component)));
}
typedef std::map<OverrideParam, float> OverrideParams;
typedef std::map<CommandListVariable*, float> OverrideVars;

class OverrideBase
{
public:
    virtual void ParseIniSection(LPCWSTR section) = 0;
};

class Override : public virtual OverrideBase
{
private:
    int transition, releaseTransition;
    TransitionType transitionType, releaseTransitionType;

    bool isConditional;
    CommandListExpression condition;

    CommandList activateCommandList;
    CommandList deactivateCommandList;

protected:
    bool active;

public:
    OverrideParams overrideParams;
    OverrideVars overrideVars;
    float overrideSeparation;
    float overrideConvergence;

    OverrideParams savedParams;
    OverrideVars savedVars;
    float userSeparation;
    float userConvergence;

    Override();
    Override(OverrideParams *params, OverrideVars *vars, float separation,
         float convergence, int transition, int release_transition,
         TransitionType transition_type,
         TransitionType release_transition_type,
         bool is_conditional, CommandListExpression condition,
         CommandList activate_command_list, CommandList deactivate_command_list) :
        overrideSeparation(separation),
        overrideConvergence(convergence),
        transition(transition),
        releaseTransition(release_transition),
        transitionType(transition_type),
        releaseTransitionType(release_transition_type),
        isConditional(is_conditional),
        condition(condition),
        activateCommandList(activate_command_list),
        deactivateCommandList(deactivate_command_list)
    {
        overrideParams = *params;
        overrideVars = *vars;
    }

    void ParseIniSection(LPCWSTR section) override;

    void Activate(HackerDevice *device, bool override_has_deactivate_condition);
    void Deactivate(HackerDevice *device);
    void Toggle(HackerDevice *device);
    bool MatchesCurrent(HackerDevice *device);
};

class KeyOverrideBase : public virtual OverrideBase, public InputListener
{
};

class KeyOverride : public KeyOverrideBase, public Override
{
private:
    KeyOverrideType type;

public:
    KeyOverride(KeyOverrideType type) :
        Override(),
        type(type)
    {}
    KeyOverride(KeyOverrideType type, OverrideParams *params, OverrideVars *vars,
            float separation, float convergence,
            int transition, int release_transition,
            TransitionType transition_type,
            TransitionType release_transition_type,
            bool is_conditional, CommandListExpression condition,
            CommandList activate_command_list, CommandList deactivate_command_list) :
        Override(params, vars, separation, convergence,
                transition, release_transition,
                transition_type, release_transition_type,
                is_conditional, condition,
                activate_command_list, deactivate_command_list),
        type(type)
    {}

    void DownEvent(HackerDevice *device);
    void UpEvent(HackerDevice *device);
#pragma warning(suppress : 4250) // Suppress ParseIniSection inheritance via dominance warning
};

class KeyOverrideCycle : public KeyOverrideBase
{
private:
    std::vector<class KeyOverride> presets;
    int current;
    bool wrap;
    bool smart;
public:
    KeyOverrideCycle() :
        current(-1),
        wrap(true),
        smart(true)
    {}

    void ParseIniSection(LPCWSTR section) override;
    void DownEvent(HackerDevice *device);
    void BackEvent(HackerDevice *device);
    void UpdateCurrent(HackerDevice *device);
};

class KeyOverrideCycleBack : public InputListener
{
    std::shared_ptr<KeyOverrideCycle> cycle;
public:
    KeyOverrideCycleBack(std::shared_ptr<KeyOverrideCycle> cycle) :
        cycle(cycle)
    {}

    void DownEvent(HackerDevice *device);
};

class PresetOverride : public Override
{
private:
    bool triggered;
    bool excluded;
    std::unordered_set<CommandListCommand*> triggersThisFrame;

public:
    PresetOverride() :
        Override(),
        triggered(false),
        excluded(false),
        uniqueTriggersRequired(0)
    {}

    void Trigger(CommandListCommand *triggered_from);
    void Exclude();
    void Update(HackerDevice *device);

    unsigned uniqueTriggersRequired;
};

// Sorted map so that if multiple presets affect the same thing the results
// will be consistent:
typedef std::map<std::wstring, class PresetOverride> PresetOverrideMap;
extern PresetOverrideMap preset_overrides;

struct OverrideTransitionParam
{
    float start;
    float target;
    ULONGLONG activation_time;
    int time;
    TransitionType transition_type;

    OverrideTransitionParam() :
        start(FLT_MAX),
        target(FLT_MAX),
        activation_time(0),
        time(-1),
        transition_type(TransitionType::LINEAR)
    {}
};

class OverrideTransition
{
public:
    std::map<OverrideParam, OverrideTransitionParam> params;
    std::map<CommandListVariable*, OverrideTransitionParam> vars;
    OverrideTransitionParam separation, convergence;

    void ScheduleTransition(HackerDevice *wrapper,
            float target_separation, float target_convergence,
            OverrideParams *targets, OverrideVars *vars,
            int time, TransitionType transition_type);
    void UpdatePresets(HackerDevice *wrapper);
    void OverrideTransition::UpdateTransitions(HackerDevice *wrapper);
    void Stop();
};

// This struct + class provides a global save for each of the overridable
// parameters. It is used to ensure that after all toggle and hold type
// bindings are released that the final value that is restored matches the
// original value. The local saves in each individual override do not guarantee
// this.
class OverrideGlobalSaveParam
{
private:
    float save;
    int refcount;
public:
    OverrideGlobalSaveParam();

    float Reset();
    void Save(float val);
    int Restore(float *val);
};

class OverrideGlobalSave
{
public:
    std::map<OverrideParam, OverrideGlobalSaveParam> params;
    std::map<CommandListVariable*, OverrideGlobalSaveParam> vars;
    OverrideGlobalSaveParam separation, convergence;

    void Reset(HackerDevice* wrapper);
    void Save(HackerDevice *wrapper, Override *preset);
    void Restore(Override *preset);
};

// We only use a single transition instance to simplify the edge cases and
// answer what happens when we have overlapping transitions - there can only be
// one active transition for each parameter.
extern OverrideTransition current_transition;
extern OverrideGlobalSave override_save;