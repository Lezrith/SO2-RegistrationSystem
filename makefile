all: registration patient doctor

registration: Registration/inf122473_r.c inf122473_vector.c inf122473_utility.c
	gcc -std=gnu99 -Wall -o inf122473_registration Registration/inf122473_r.c inf122473_vector.c inf122473_utility.c -lm -I .

patient: Patient/inf122473_p.c inf122473_utility.c
	gcc -std=gnu99 -Wall -o inf122473_patient Patient/inf122473_p.c inf122473_utility.c -lm -I .
	
doctor: Doctor/inf122473_l.c inf122473_utility.c
	gcc -std=gnu99 -Wall -o inf122473_doctor Doctor/inf122473_l.c inf122473_utility.c -lm -I .