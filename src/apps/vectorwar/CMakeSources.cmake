set(GGPO_EXAMPLES_VECTORWAR_INC_NOFILTER
	"gamestate.h"
	"gtk_renderer.h"
	"ggpo_perfmon.h"
	"nongamestate.h"
	"renderer.h"
	# "Resource.h"
	# "targetver.h"
	"vectorwar.h"
)

set(GGPO_EXAMPLES_VECTORWAR_SRC_NOFILTER
	"gamestate.cpp"
	"gtk_renderer.cpp"
	"ggpo_perfmon.cpp"
	"main.cpp"
	"vectorwar.cpp"
)


source_group(" " FILES ${GGPO_EXAMPLES_VECTORWAR_INC_NOFILTER} ${GGPO_EXAMPLES_VECTORWAR_SRC_NOFILTER})

set(GGPO_EXAMPLES_VECTORWAR_SRC
	${GGPO_EXAMPLES_VECTORWAR_INC_NOFILTER}
	${GGPO_EXAMPLES_VECTORWAR_SRC_NOFILTER}
)