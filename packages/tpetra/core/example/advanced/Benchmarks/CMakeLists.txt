IF (Tpetra_INST_INT_INT)
  # This example requires GlobalOrdinal = int, so that Epetra and
  # Tpetra can use the same global index lists.  It would not be hard
  # to make this example use the default GlobalOrdinal type instead.
  TRIBITS_ADD_EXECUTABLE(
    import
    SOURCES import.cpp
    COMM mpi
    )
ENDIF ()

TRIBITS_ADD_EXECUTABLE(
  vector
  SOURCES vector.cpp
  COMM serial mpi
)

TRIBITS_ADD_EXECUTABLE(
  blockCrsMatrixMatVec
  SOURCES blockCrsMatrixMatVec.cpp
  COMM serial mpi
)

TRIBITS_ADD_EXECUTABLE(
  localView
  SOURCES localView.cpp
  COMM serial mpi
)

TRIBITS_ADD_EXECUTABLE(
  CrsMatrix_sumIntoLocalValues
  SOURCES CrsMatrix_sumIntoLocalValues.cpp
  COMM serial mpi
)
