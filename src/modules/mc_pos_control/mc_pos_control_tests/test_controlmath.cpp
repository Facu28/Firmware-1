#include <unit_test.h>
#include "../Utility/ControlMath.hpp"
#include <mathlib/mathlib.h>

static const float EPS = 0.00000001f;

class ControlMathTest : public UnitTest
{
public:
	virtual bool run_tests();

private:
	bool testConstrainTilt();
	bool testConstrainPIDu();
	bool testThrAttMapping();


};

bool ControlMathTest::run_tests()
{
	ut_run_test(testConstrainTilt);
	ut_run_test(testConstrainPIDu);
	ut_run_test(testThrAttMapping);

	return (_tests_failed == 0);
}

bool ControlMathTest::testConstrainTilt()
{
	// expected: return same vector
	// reason: tilt exceeds maximum tilt
	matrix::Vector3f v(0.5f, 0.5f, 0.1f);
	float tilt_max = math::radians(91.0f);
	matrix::Vector3f vr = ControlMath::constrainTilt(v, tilt_max);
	ut_assert_true((v - vr).length() < EPS);

	// expected: return zero vector
	// reason: v points down, but cone generated by tilt is only
	// defined in negative z (upward).
	v = matrix::Vector3f(1.0f, 1.0f, 0.1f);
	tilt_max = math::radians(45.0f);
	vr = ControlMath::constrainTilt(v, tilt_max);
	ut_assert_true((vr).length() < EPS);

	// expected: length vr_xy same as vr_z
	// reason: it is a 45 cone and v_xy exceeds v_z
	v = matrix::Vector3f(1.0f, 1.0f, -0.5f);
	tilt_max = math::radians(45.0f);
	vr = ControlMath::constrainTilt(v, tilt_max);
	float vr_xy = matrix::Vector2f(vr(0), vr(1)).length();
	ut_assert_true(fabsf(vr(2)) - vr_xy < EPS);

	// expected: length vr_z larger than vr_xy
	// reason: it is a 30 cone and v_xy exceeds v_z
	v = matrix::Vector3f(1.0f, 1.0f, -0.5f);
	tilt_max = math::radians(20.0f);
	vr = ControlMath::constrainTilt(v, tilt_max);
	vr_xy = matrix::Vector2f(vr(0), vr(1)).length();
	ut_assert_true(fabsf(vr(2)) - vr_xy > EPS);

	// expected: length of vr_xy larger than vr_z
	// reason: it is a 80 cone and v_xy exceeds v_z
	v = matrix::Vector3f(10.0f, 10.0f, -0.5f);
	tilt_max = math::radians(80.f);
	vr = ControlMath::constrainTilt(v, tilt_max);
	vr_xy = matrix::Vector2f(vr(0), vr(1)).length();
	ut_assert_true(fabsf(vr(2)) - vr_xy < EPS);

	// expected: same vector is return
	// reson: vector is within cond
	v = matrix::Vector3f(1.0f, 1.0f, -0.5f);
	tilt_max = math::radians(89.f);
	vr = ControlMath::constrainTilt(v, tilt_max);
	ut_assert_true((v - vr).length() < EPS);

	return true;

}

bool ControlMathTest::testConstrainPIDu()
{
	/* Notation:
	 * u: input thrust that gets modified
	 * u_o: unmodified thrust input
	 * sat: saturation flags
	 * Ulim: max and min thrust
	 * d: flags for xy and z, indicating sign of (r-y)
	 * r: reference; not used here
	 * y: measurement; not used here
	 */

	// expected: same u
	// reason: no direction change and within bounds
	bool sat[2] = {false, false};
	float Ulim[2] = {0.8f, 0.2f};
	matrix::Vector3f u{0.1f, 0.1f, -0.4f};
	matrix::Vector3f u_o = u;
	float d[2] = {1.0f, 1.0f};
	ControlMath::constrainPIDu(u, sat, Ulim, d);
	ut_assert_true((u - u_o).length() < EPS);
	ut_assert_false(sat[1]);
	ut_assert_false(sat[0]);

	// expected: u_xy smaller than u_o_xy and sat[0] = true
	// reason: u_o_xy exceeds Ulim[0] and d[0] is positive
	sat[0] = false;
	sat[1] = false;
	Ulim[0] = 0.5f;
	Ulim[1] = 0.2f;
	u = matrix::Vector3f(0.4f, 0.4f, -0.1f);
	u_o = u;
	d[0] = 1.0f;
	d[1] = 1.0f;
	ControlMath::constrainPIDu(u, sat, Ulim, d);
	float u_xy = matrix::Vector2f(u(0), u(1)).length();
	float u_o_xy = matrix::Vector2f(u_o(0), u_o(1)).length();
	ut_assert_true(u_xy < u_o_xy);
	ut_assert_true(fabsf(u(2)) - fabsf(u_o(2)) < EPS);
	ut_assert_true(sat[0]);
	ut_assert_false(sat[1]);

	// expected: u_xy smaller than u_o_xy and sat[0] = false
	// reason: u_o_xy exceeds Ulim[0] and d[0] is negative
	d[0] = -1.0f;
	ut_assert_true(u_xy < u_o_xy);
	ut_assert_true(fabsf(u(2)) - fabsf(u_o(2)) < EPS);
	ut_assert_true(sat[0]);
	ut_assert_false(sat[1]);

	// expected: u_xy = 0  and sat[0] = true
	// expected: u_z = -0.6 (maximum) and sat[1] = true
	// reason: u_o_z exceeds maximum and since altitude
	// has higher priority, u_xy will be set to 0. No direction
	// change desired.
	sat[0] = false;
	sat[1] = false;
	Ulim[0] = 0.5f;
	Ulim[1] = 0.2f;
	u = matrix::Vector3f(0.4f, 0.4f, -0.6f);
	u_o = u;
	d[0] = 1.0f;
	d[1] = 1.0f;
	ControlMath::constrainPIDu(u, sat, Ulim, d);
	u_xy = matrix::Vector2f(u(0), u(1)).length();
	u_o_xy = matrix::Vector2f(u_o(0), u_o(1)).length();
	ut_assert_true(u_xy < u_o_xy);
	ut_assert_true(u_o(2) - (-0.6f) < EPS);
	ut_assert_true(u_xy < EPS);
	ut_assert_true(sat[0]);
	ut_assert_true(sat[1]);

	// expected: u_xy = 0  and sat[0] = true because u_z is saturate
	// => altitude priority
	// expected: u_z = -0.6 (maximum) and sat[1] = true
	// reason: u_o_z exceeds maximum and since altitude
	// has higher priority, u_xy will be set to 0. Direction
	// change desired for xy
	d[0] = -1.0f;
	ControlMath::constrainPIDu(u, sat, Ulim, d);
	u_xy = matrix::Vector2f(u(0), u(1)).length();
	u_o_xy = matrix::Vector2f(u_o(0), u_o(1)).length();
	ut_assert_true(u_xy < u_o_xy);
	ut_assert_true(u_o(2) - (-0.6f) < EPS);
	ut_assert_true(u_xy < EPS);
	ut_assert_true(sat[0]);
	ut_assert_true(sat[1]);

	// expected: nothing
	// reason: thottle within bounds
	sat[0] = false;
	sat[1] = false;
	Ulim[0] = 0.7f;
	Ulim[1] = 0.2f;
	u = matrix::Vector3f(0.3f, 0.3f, 0.0f);
	u_o = u;
	d[0] = 1.0f;
	d[1] = 1.0f;
	ControlMath::constrainPIDu(u, sat, Ulim, d);
	ut_assert_true((u - u_o).length() < EPS);
	ut_assert_false(sat[1]);
	ut_assert_false(sat[0]);

	// expected: u_xy at minimum, no saturation
	// reason: u_o is below minimum with u_o_z = 0, which
	// 		   means that Ulim[1] is in xy direction.
	//         No saturation because no direction change.
	sat[0] = false;
	sat[1] = false;
	Ulim[0] = 0.7f;
	Ulim[1] = 0.2f;
	u = matrix::Vector3f(0.05f, 0.05f, 0.0f);
	u_o = u;
	d[0] = 1.0f;
	d[1] = 1.0f;
	ControlMath::constrainPIDu(u, sat, Ulim, d);
	u_xy = matrix::Vector2f(u(0), u(1)).length();
	ut_assert_true(u_xy - Ulim[1] < EPS);
	ut_assert_true(fabsf(u(2)) < EPS);
	ut_assert_false(sat[1]);
	ut_assert_false(sat[0]);

	// expected: u_xy at minimum, saturation in z
	// reason: u_o is below minimum with u_o_z = 0, which
	// 		   means that Ulim[1] is in xy direction.
	//         Direction change in z.
	d[1] = -1.0f;
	ControlMath::constrainPIDu(u, sat, Ulim, d);
	ut_assert_true(u_xy - Ulim[1] < EPS);
	ut_assert_true(fabsf(u(2)) < EPS);
	ut_assert_true(sat[1]);
	ut_assert_false(sat[0]);

	return true;
}

bool ControlMathTest::testThrAttMapping()
{

	/* expected: zero roll, zero pitch, zero yaw, full thr mag
	 * reasone: thrust pointing full upward
	 */
	matrix::Vector3f thr{0.0f, 0.0f, -1.0f};
	float yaw = 0.0f;
	vehicle_attitude_setpoint_s att = ControlMath::thrustToAttitude(thr, yaw);
	ut_compare("", att.roll_body, EPS);
	ut_assert_true(att.pitch_body < EPS);
	ut_assert_true(att.yaw_body < EPS);
	ut_assert_true(att.thrust - 1.0f < EPS);

	/* expected: same as before but with 90 yaw
	 * reason: only yaw changed
	 */
	yaw = M_PI_2_F;
	att = ControlMath::thrustToAttitude(thr, yaw);
	ut_assert_true(att.roll_body < EPS);
	ut_assert_true(att.pitch_body < EPS);
	ut_assert_true(att.yaw_body - M_PI_2_F < EPS);
	ut_assert_true(att.thrust - 1.0f < EPS);

	/* expected: same as before but roll 180
	 * reason: thrust points straight down and order Euler
	 * order is: 1. roll, 2. pitch, 3. yaw
	 */
	thr = matrix::Vector3f(0.0f, 0.0f, 1.0f);
	att = ControlMath::thrustToAttitude(thr, yaw);
	ut_assert_true(fabsf(att.roll_body) - M_PI_F < EPS);
	ut_assert_true(fabsf(att.pitch_body) < EPS);
	ut_assert_true(att.yaw_body - M_PI_2_F < EPS);
	ut_assert_true(att.thrust - 1.0f < EPS);

	/* TODO: find a good way to test it */


	return true;
}

ut_declare_test_c(test_controlmath, ControlMathTest)
