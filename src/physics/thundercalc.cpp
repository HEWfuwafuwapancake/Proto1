#include "physics/thundercalc.h"

static VEC3 BodyCenter(const CAPSULE& body)
{
	return Vec3Scale(Vec3Add(body.Start, body.End), 0.5f);
}

int ThunderCalcChain(const TENTACLE_TARGET* targets, int count, int first, float range, int maxCount, int* order)
{
	if (first < 0 || first >= count || maxCount <= 0) return 0;
	int written = 0;
	order[written++] = first;
	while (written < maxCount) {
		const VEC3 from = BodyCenter(targets[order[written - 1]].Body);
		int best = -1;
		float bestDistance = range;
		for (int i = 0; i < count; ++i) {
			if (!targets[i].Available) continue;
			bool used = false;
			for (int k = 0; k < written; ++k) used = used || order[k] == i;
			if (used) continue;
			const float d = Vec3Length(Vec3Sub(BodyCenter(targets[i].Body), from));
			if (d <= bestDistance) {
				best = i;
				bestDistance = d;
			}
		}
		if (best < 0) break;
		order[written++] = best;
	}
	return written;
}
