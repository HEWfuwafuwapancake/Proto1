#include "physics/armusecalc.h"

#include <algorithm>

void ArmUseCalc::Reset(int maxUses)
{
	m_Remaining = std::max(0, maxUses);
}

void ArmUseCalc::Consume(int count)
{
	m_Remaining -= std::min(m_Remaining, std::max(0, count));
}
