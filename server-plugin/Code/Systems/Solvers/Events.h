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

/*
	Definitions of events the testers can send to the solvers.

	These events are then placed in an history to form a sort of fingerprint of the current play pattern.
	If we found the signature of a cheat in this fingerprint, then we send a detection.

	Unlike testers who detect a single form of cheat, here we are gathering all tester's detections, including the most tiny ones.

	In short, say welcome to the primitive of Machine Learning ... muuaaahahahahaha.
*/

#include <cstdint>

typedef uint64_t solver_event_enum_type;

enum SolverEvents : solver_event_enum_type
{
	NOTHING = 0, // This is not gonna be used, will kind of break signatures

	// Aim events

	SHOOT = 1,
	HIT_TARGET = 2,
	THRU_WALL = 4,
	PERFECT_RECOIL = 8,
	ALMOST_PERFECT_RECOIL = 16,
	TARGET_SWITCH = 32,
	DETECT_MOUSEMISMATCH = 64,
	DETECT_ANGLESNAP = 128,
	DETECT_ANGLEOUTOFBOUND = 256,
	SHORT_REACT_TIME = 512,
	AIM_STICK = 1024,
	DETECT_RAPIDFIRE = 32768,

	// Movement events

	DETECT_STRAFE = 2048,
	DETECT_PERFECT_BHOP = 4096,
	DETECT_SCRIPT_BHOP = 8192,
	DETECT_TAMPERING_USERCMD = 16384

};

/* Game events stacktraces

CGameClient::ProcessMove
	CServerGameClients::ProcessUsercmds
		CBasePlayer::ProcessUsercmds
			CBasePlayer::PhysicsSimulate
				CCSPlayer::PlayerRunCommand
					CBasePlayer::PlayerRunCommand
						CPlayerMove::RunCommand
							CGameMovement::ProcessMovement
								CGameMovement::PlayerMove
									CCSGameMovement::CheckParameters
										CCSGameMovement::DecayPunchAngle
							CPlayerMove::RunPostThink
								CBasePlayer::PostThink
									CBasePlayer::ItemPostFrame
										Weapon::ItemPostFrame
											weapon_fire
											weapon_fire_on_empty
											Weapon::PrimaryAttack()
												CWeaponCSBaseGun::CSBaseGunFire
													FX_FireBullets
														StartLagCompensation
														CCSPlayer::FireBullet
															bullet_impact
															CBaseEntity::DispatchTraceAttack
																CBaseEntity::TraceAttack
																	AddMultiDamage
																		ApplyMultiDamage
																			CBaseEntity::TakeDamage
																				CCSPlayer::OnTakeDamage
																					CBaseCombatCharacter::OnTakeDamage
																						CCSPlayer::OnTakeDamage_Alive
																							player_hurt
																						CBaseCombatCharacter::Event_Killed
																							UTIL_Remove
																								CBaseCombatCharacter::UpdateOnRemove
																									CCSGameRules::DeathNotice
																										payer_death
														FinishLagCompensation
												CCSPlayer::KickBack
													CBasePlayer::SetPunchAngle
*/
