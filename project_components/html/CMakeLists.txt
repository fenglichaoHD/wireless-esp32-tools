file(GLOB HTML_FILES
        *.gz
        )

idf_component_register(
        EMBED_FILES ${HTML_FILES}
)
