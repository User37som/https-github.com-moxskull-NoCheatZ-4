/*
	Copyright 2012 - Le Padellec Sylvain

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef AIMTESTER_H
#define AIMTESTER_H

/*
	This one is tough ...

	We will try to detect "human-like" aimbots automatically.

	- Need to implement "aiming good" probability model
		- Detect accuracy of spray pattern over visible target and over invisible target (basically tells wether the current shot is near the last one over that target; more the pattern tells that bulllet was supposed to be far the last one, more the probability of cheating is true if that bullet is near the last one ... always over that target)
			- Have the weapon recoil values
			- Properly detect all visibles targets (move visibility data struct from antiwallhackblocker to a persistent system)
			- Detect the current target and sray transfers
				- Have a relative reference 3dpoint to that target (like a bone)
					- Capture body impact
				- Have a distance to that target
				- Have the velocity of that target
				- Have the number of shots fired
					
	- Detect inconsistent aim speed towards a target
		- That happens when the crosshair enters the aimbot radio of a target.
		- Also very fast and multiple aim direction changes within a short period.

	Will use the throwback data struct, registering everything for 1 second.
		- Implement .bmp graphical print of a detection's data.

*/

#endif // AIMTESTER_H
