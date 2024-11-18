						.386p

						PUBLIC		COUNT20000

						ASSUME CS:CODE

CODE					SEGMENT


COUNT20000				PROC

						PUSHFD
						CLI

						PUSH	ECX
						PUSH	EDX

						MOV		ECX,20000

						IN		AX,0026H	; Free-Run Timer since UG
						MOV		DX,AX

WAIT_20000US:			OUT		06CH,AL		; Wait 1us since 20F
						LOOP	WAIT_20000US

						IN		AX,0026H	; Free-Run Timer since UG
						SUB		AX,DX
						MOVZX	EAX,AX

						POP		EDX
						POP		ECX

						POPFD

						RET

COUNT20000				ENDP


CODE					ENDS
						END
