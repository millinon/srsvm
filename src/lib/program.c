#include <stdlib.h>
#include <string.h>

#include "srsvm/debug.h"
#include "srsvm/program.h"
#include "srsvm/debug.h"

void srsvm_program_free_metadata(srsvm_program_metadata *metadata)
{
    if(metadata != NULL){
        free(metadata);
    }
}

#if defined(WORD_SIZE)
void srsvm_program_free_register(srsvm_register_specification *reg)
{
    if(reg != NULL){
        if(reg->next != NULL){
            srsvm_program_free_register(reg->next);
        }

        free(reg);
    }
}

void srsvm_program_free_vmem(srsvm_virtual_memory_specification *vmem)
{
    if(vmem != NULL){
        if(vmem->next != NULL){
            srsvm_program_free_vmem(vmem->next);
        }

        free(vmem);
    }
}

void srsvm_program_free_lmem(srsvm_literal_memory_specification *lmem)
{
    if(lmem != NULL){
        if(lmem->next != NULL){
            srsvm_program_free_lmem(lmem->next);
        }

        free(lmem);
    }
}

void srsvm_program_free_const(srsvm_constant_specification *c)
{
    if(c != NULL){
        if(c->next != NULL){
            srsvm_program_free_const(c->next);
        }

        free(c);
    }
}
#endif

void srsvm_program_free(srsvm_program *program)
{
    if(program != NULL){
        if(program->metadata != NULL){
            srsvm_program_free_metadata(program->metadata);
        }

#if defined(WORD_SIZE)
        if(program->registers != NULL){
            srsvm_program_free_register(program->registers);
        }

        if(program->virtual_memory != NULL){
            srsvm_program_free_vmem(program->virtual_memory);
        }

        if(program->literal_memory != NULL){
            srsvm_program_free_lmem(program->literal_memory);
        }

        if(program->constants != NULL){
            srsvm_program_free_const(program->constants);
        }
#endif
    }
}

srsvm_program *srsvm_program_alloc(void)
{
    srsvm_program *program = malloc(sizeof(srsvm_program));

    if(program != NULL){
        memset(program, 0, sizeof(srsvm_program));
    }

    return program;
}


srsvm_program_metadata* srsvm_program_metadata_alloc(void)
{
    srsvm_program_metadata *metadata = malloc(sizeof(srsvm_program_metadata));

    if(metadata != NULL){
        memset(metadata, 0, sizeof(srsvm_program_metadata));
    }

    return metadata;
}


#if defined(WORD_SIZE)
srsvm_register_specification* srsvm_program_register_alloc(void)
{
    srsvm_register_specification* reg = malloc(sizeof(srsvm_register_specification));

    if(reg != NULL){
        memset(reg, 0, sizeof(srsvm_register_specification));
    }

    return reg;
}

srsvm_virtual_memory_specification* srsvm_program_vmem_alloc(void)
{
    srsvm_virtual_memory_specification* vmem = malloc(sizeof(srsvm_virtual_memory_specification));

    if(vmem != NULL){
        memset(vmem, 0, sizeof(srsvm_virtual_memory_specification));
    }

    return vmem;
}

srsvm_literal_memory_specification* srsvm_program_lmem_alloc(void)
{
    srsvm_literal_memory_specification *lmem = malloc(sizeof(srsvm_literal_memory_specification));

    if(lmem != NULL){
        memset(lmem, 0, sizeof(srsvm_literal_memory_specification));
    }

    return lmem;
}

srsvm_constant_specification* srsvm_program_const_alloc(void)
{
    srsvm_constant_specification *c = malloc(sizeof(srsvm_constant_specification));

    if(c != NULL){
        memset(c, 0, sizeof(srsvm_constant_specification));
    }

    return c;
}
#endif

static bool deserialize_metadata(FILE *stream, srsvm_program *program)
{
    bool success = false;

    srsvm_program_metadata* metadata = NULL;

    if(program != NULL){
        if(stream != NULL && (metadata = srsvm_program_metadata_alloc()) != NULL){
            size_t read_result = fread(&metadata->word_size, sizeof(metadata->word_size), 1, stream);

            if(read_result < 1){
                goto error_cleanup;
            }

            if(metadata->word_size == '#'){
                char buf;

                read_result = fread(&buf, sizeof(char), 1, stream);

                if(read_result == 1 && buf == '!'){
                    metadata->shebang[0] = '#';
                    metadata->shebang[1] = '!';

                    if(fgets(metadata->shebang + 2, PATH_MAX-2, stream) == metadata->shebang + 2){
                        read_result = fread(&metadata->word_size, sizeof(metadata->word_size), 1, stream);

                        if(read_result != 1){
                            goto error_cleanup;
                        } else {
#if defined(WORD_SIZE)
                            if(fread(&metadata->entry_point, sizeof(metadata->entry_point), 1, stream) < 1){
                                goto error_cleanup;
                            } else {
                                success = true;
                            }
#else
                            success = true;
#endif
                        }
                    } else goto error_cleanup;
                } else goto error_cleanup;
            } else {
#if defined(WORD_SIZE)
                if(fread(&metadata->entry_point, sizeof(metadata->entry_point), 1, stream) < 1){
                    goto error_cleanup;
                } else {
                    success = true;
                }
#else
                success = true;
#endif
            }
        }

        if(success){
            program->metadata = metadata;
        }
    }


    return success;

error_cleanup:
    if(metadata != NULL){
        free(metadata);
    }

    return false;
}

#if defined(WORD_SIZE)

static bool deserialize_registers(FILE *stream, srsvm_program *program)
{
    bool success = false;

    srsvm_register_specification *reg = NULL, *last_reg = NULL;

    bool claimed_slots[SRSVM_REGISTER_MAX_COUNT] = { false };

    if(stream != NULL && program != NULL){
        if(fread(&program->num_registers, sizeof(program->num_registers), 1, stream) == 1){
            if(program->num_registers > SRSVM_REGISTER_MAX_COUNT){
                goto error_cleanup;
            }

            for(uint16_t i = 0; i < program->num_registers; i++){
                reg = srsvm_program_register_alloc();

                if(reg != NULL){
                    if(fread(&reg->name, sizeof(char), SRSVM_REGISTER_MAX_NAME_LEN, stream) != SRSVM_REGISTER_MAX_NAME_LEN){
                        goto error_cleanup;
                    } else if(fread(&reg->index, sizeof(reg->index), 1, stream) != 1){
                        goto error_cleanup;
                    } else if(reg->index > SRSVM_REGISTER_MAX_COUNT || claimed_slots[reg->index]){
                        goto error_cleanup;
                    } else {
                        if(program->registers == NULL){
                            program->registers = reg;
                        } else {
                            last_reg->next = reg;
                        }
                        last_reg = reg;
                        claimed_slots[reg->index] = true;
                    }
                } else goto error_cleanup;
            }

            success = true;
        } else goto error_cleanup;
    }

    return success;

error_cleanup:
    if(reg != NULL){
        srsvm_program_free_register(reg);
    }

    if(program->registers != NULL){
        srsvm_program_free_register(program->registers);
    }

    return false;
}

static bool deserialize_vmem(FILE *stream, srsvm_program *program)
{
    bool success = false;

    srsvm_virtual_memory_specification *vmem = NULL, *last_vmem; 

    if(stream != NULL && program != NULL){
        size_t read_result = fread(&program->num_vmem_segments, sizeof(program->num_vmem_segments), 1, stream);

        if(read_result < 1){
            goto error_cleanup;
        } else {
            for(uint16_t i = 0; i < program->num_vmem_segments; i++){
                vmem = srsvm_program_vmem_alloc();

                if(vmem != NULL){
                    if(fread(&vmem->start_address, sizeof(vmem->start_address), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&vmem->size, sizeof(vmem->size), 1, stream) < 1){
                        goto error_cleanup;
                    } else {
                        if(program->virtual_memory == NULL){
                            program->virtual_memory = vmem;    
                        } else {
                            last_vmem->next = vmem;
                        }

                        last_vmem = vmem;
                    }
                } else goto error_cleanup;
            }

            success = true;
        }
    }

    return success;

error_cleanup:
    if(vmem != NULL){
        srsvm_program_free_vmem(vmem);
    }
    if(program->virtual_memory != NULL){
        srsvm_program_free_vmem(program->virtual_memory);
    }

    return false;
}

static bool deserialize_lmem(FILE *stream, srsvm_program *program)
{
    bool success = false;

    srsvm_literal_memory_specification *lmem = NULL, *last_lmem; 

    if(stream != NULL && program != NULL){
        size_t read_result = fread(&program->num_lmem_segments, sizeof(program->num_lmem_segments), 1, stream);

        if(read_result < 1){
            goto error_cleanup;
        } else {
            for(uint16_t i = 0; i < program->num_lmem_segments; i++){
                lmem = srsvm_program_lmem_alloc();

                if(lmem != NULL){
                    if(fread(&lmem->start_address, sizeof(lmem->start_address), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&lmem->size, sizeof(lmem->size), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&lmem->is_compressed, sizeof(lmem->is_compressed), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&lmem->compressed_size, sizeof(lmem->compressed_size), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&lmem->readable, sizeof(lmem->readable), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&lmem->writable, sizeof(lmem->writable), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&lmem->executable, sizeof(lmem->readable), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(fread(&lmem->locked, sizeof(lmem->locked), 1, stream) < 1){
                        goto error_cleanup;
                    } else {
                        size_t read_bytes;
                        if(lmem->is_compressed){
                            read_bytes = (size_t) lmem->compressed_size;
                        } else {
                            read_bytes = (size_t) lmem->size;
                        }
                        
                        lmem->data = malloc(read_bytes);

                        if(lmem->data == NULL){
                            goto error_cleanup;
                        }

                        if(fread(lmem->data, sizeof(char), read_bytes, stream) < read_bytes){
                            goto error_cleanup;
                        }

                        if(lmem->is_compressed){
#if defined(SRSVM_SUPPORT_COMPRESSED_MEMORY)
                            void *compressed_data = lmem->data;
                            lmem->data = NULL;

                            size_t compressed_size = (size_t) lmem->compressed_size;
                            size_t original_size = (size_t) lmem->size;

                            void *decompressed_data = srsvm_zlib_inflate(compressed_data, compressed_size, original_size);

                            free(compressed_data);

                            if(decompressed_data == NULL){
                                goto error_cleanup;
                            }

                            lmem->data = decompressed_data;
#else
                            dbg_puts("ERROR: Tried to load a program with compressed memory, which this implementation does not support");
                            goto error_cleanup;
#endif
                        }
                        if(program->literal_memory == NULL){
                            program->literal_memory = lmem;
                        } else {
                            last_lmem->next = lmem;
                        }
                        last_lmem = lmem;
                    }
                } else goto error_cleanup;
            }

            success = true;
        }
    }

    return success;

error_cleanup:
    if(lmem != NULL){
        srsvm_program_free_lmem(lmem);
    }
    if(program->literal_memory != NULL){
        srsvm_program_free_lmem(program->literal_memory);
    }

    return false;
}

static bool deserialize_constants(FILE *stream, srsvm_program *program)
{
    bool success = false;

    srsvm_constant_specification *c = NULL, *last_c; 

    bool claimed_slots[SRSVM_CONST_MAX_COUNT] = { false };

    if(stream != NULL && program != NULL){
        size_t read_result = fread(&program->num_constants, sizeof(program->num_constants), 1, stream);

        if(read_result < 1){
            dbg_puts("ERROR: failed to read number of constants");
            goto error_cleanup;
        } else {
            for(uint16_t i = 0; i < program->num_constants; i++){
                c = srsvm_program_const_alloc();

                if(c != NULL){
                    if(fread(&c->const_slot, sizeof(c->const_slot), 1, stream) < 1){
                        goto error_cleanup;
                    } else if(c->const_slot > SRSVM_CONST_MAX_COUNT || claimed_slots[c->const_slot]){
                        goto error_cleanup;
                    } else if(fread(&c->const_val.type,  sizeof(c->const_val.type), 1, stream) < 1){
                        goto error_cleanup;
                    } else {
                        switch(c->const_val.type)
                        {

#define LOADER(field,flag) case flag:\
                            if(fread(&c->const_val.field, sizeof(c->const_val.field), 1, stream) < 1){ \
                                goto error_cleanup; \
                            } \
                            break; 

                            LOADER(word, WORD);
                            LOADER(ptr, PTR);
                            LOADER(ptr_offset, PTR_OFFSET);
                            LOADER(bit, BIT);
                            LOADER(u8, U8);
                            LOADER(i8, I8);
                            LOADER(u16, U16);
                            LOADER(i16, I16);
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
                            LOADER(u32, U32);
                            LOADER(i32, I32);
                            LOADER(f32, F32);
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
                            LOADER(u64, U64);
                            LOADER(i64, I64);
                            LOADER(f64, F64);
#endif
#if WORD_SIZE == 128
                            LOADER(u128, U128);
                            LOADER(i128, I128);
#endif
#undef LOADER
                            case STR:
                                if(fread(&c->const_val.str_len, sizeof(c->const_val.str_len), 1, stream) != 1){
                                    dbg_puts("ERROR: failed to read string length");
                                    goto error_cleanup;
                                } else {
                                    char *tmp = malloc((c->const_val.str_len + 1) * sizeof(char));

                                    if(tmp == NULL){
                                        goto error_cleanup;
                                    } else {
                                        memset(tmp, 0, (size_t) c->const_val.str_len + 1);
                                    }

                                    if(fread(tmp, sizeof(char), (size_t) c->const_val.str_len, stream) != c->const_val.str_len){
                                        dbg_puts("ERROR: failed to read string");

                                        free(tmp);
                                        goto error_cleanup;
                                    }

                                    c->const_val.str = (const char*) tmp;
                                }
                                break;

                            default:
                            goto error_cleanup;
                        }

                        if(program->constants == NULL){
                            program->constants = c;
                        } else {
                            last_c->next = c;
                        }

                        last_c = c;
                        claimed_slots[c->const_slot] = true;
                    }
                } else goto error_cleanup;
            }

            success = true;
        }
    }

    return success;

error_cleanup:
    if(c != NULL){
        srsvm_program_free_const(c);
    }
    if(program->constants != NULL){
        srsvm_program_free_const(program->constants);

    }

    return false;
}

srsvm_program *srsvm_program_deserialize(const char* program_path)
{
    srsvm_program *program = NULL;

    FILE *stream = NULL;

    if(program_path != NULL){
        stream = fopen(program_path, "rb");

        if(stream != NULL){
            program = srsvm_program_alloc();

            if(! deserialize_metadata(stream, program)){
                dbg_puts("ERROR: failed to deserialize metadata");
                goto error_cleanup;
            } else if(! deserialize_registers(stream, program)){
                dbg_puts("ERROR: failed to deserialize registers");
                goto error_cleanup;
            } else if(! deserialize_vmem(stream, program)){
                dbg_puts("ERROR: failed to deserialize vmem");
                goto error_cleanup;
            } else if(! deserialize_lmem(stream, program)){
                dbg_puts("ERROR: failed to deserialize lmem");
                goto error_cleanup;
            } else if(! deserialize_constants(stream, program)){
                dbg_puts("ERROR: failed to deserialize constants");
                goto error_cleanup;
            }

            fclose(stream);
        }
    }

    return program;

error_cleanup:
    if(stream != NULL)
        fclose(stream);
    if(program != NULL)
        srsvm_program_free(program);

    return NULL;
}

bool serialize_metadata(FILE *stream, const srsvm_program *program)
{
    bool success = false;

    size_t shebang_len = strlen(program->metadata->shebang);
    char *writable_shebang = NULL;

    if(shebang_len > 0){
        writable_shebang = strdup(program->metadata->shebang);

        if(writable_shebang == NULL){
            goto error_cleanup;
        } 
        
        for(int i = shebang_len-1; i >= 0; i--){
            if(writable_shebang[i] == '\n'){
                writable_shebang[i] = '\0';
            } else if(writable_shebang[i] != '\0') break;
        }

        if(fprintf(stream, "%*s\n", PATH_MAX-1, writable_shebang) <= 0){
            goto error_cleanup;    
        }

        free(writable_shebang);
    }

    if(fwrite(&program->metadata->word_size, sizeof(program->metadata->word_size), 1, stream) != 1){
        goto error_cleanup;
    } else if(fwrite(&program->metadata->entry_point, sizeof(program->metadata->entry_point), 1, stream) != 1){
        goto error_cleanup;
    }

    success = true;

    return success;

error_cleanup:
    if(writable_shebang != NULL){
        free(writable_shebang);
    }

    return false;
}

bool write_reg(FILE* stream, const srsvm_register_specification *reg)
{
    if(reg == NULL){
        return true;
    } else if(fwrite(&reg->name, sizeof(char), sizeof(reg->name), stream) != sizeof(reg->name)){
        return false;
    } else if(fwrite(&reg->index, sizeof(reg->index), 1, stream) != 1){
        return false;
    } else if(reg->next == NULL){
        return true;
    } else return write_reg(stream, reg->next);
}

bool serialize_registers(FILE *stream, const srsvm_program *program)
{
    bool success = false;

    if(fwrite(&program->num_registers, sizeof(program->num_registers), 1, stream) != 1){
        success = false;
    } else {
        success = write_reg(stream, program->registers);
    }
    
    return success;
}

bool write_vmem(FILE *stream, const srsvm_virtual_memory_specification *vmem)
{
    if(vmem == NULL){
        return true;
    } else if(fwrite(&vmem->start_address, sizeof(vmem->start_address), 1, stream) != 1){
        return false;
    } else if(fwrite(&vmem->size, sizeof(vmem->size), 1, stream) != 1){
        return false;
    } else if(vmem->next == NULL){
        return true;
    } else return write_vmem(stream, vmem->next);
}

bool serialize_vmem(FILE *stream, const srsvm_program *program)
{
    bool success = false;

    if(fwrite(&program->num_vmem_segments, sizeof(program->num_vmem_segments), 1, stream) != 1){
        success = false;
    } else {
        success = write_vmem(stream, program->virtual_memory);
    }

    return success;
}

bool write_lmem(FILE *stream, const srsvm_literal_memory_specification *lmem)
{
    if(lmem == NULL){
        return true;
    } else if(fwrite(&lmem->start_address, sizeof(lmem->start_address), 1, stream) != 1){
        return false;
    } else if(fwrite(&lmem->size, sizeof(lmem->size), 1, stream) != 1){
        return false;
    } else if(fwrite(&lmem->is_compressed, sizeof(lmem->is_compressed), 1, stream) != 1){
        return false;
    } else if(fwrite(&lmem->compressed_size, sizeof(lmem->compressed_size), 1, stream) != 1){
        return false;
    } else if(fwrite(&lmem->readable, sizeof(lmem->readable), 1, stream) != 1){
        return false;
    } else if(fwrite(&lmem->writable, sizeof(lmem->writable), 1, stream) != 1){
        return false;
    } else if(fwrite(&lmem->executable, sizeof(lmem->executable), 1, stream) != 1){
        return false;
    } else if(fwrite(&lmem->locked, sizeof(lmem->locked), 1, stream) != 1){
        return false;
    } else {
        if(lmem->is_compressed){
            if(fwrite(lmem->data, sizeof(char), (size_t) lmem->compressed_size, stream) != lmem->compressed_size){
                return false;
            }
        } else {
            if(fwrite(lmem->data, sizeof(char), (size_t) lmem->size, stream) != lmem->size){
                return false;
            }
        }

        if(lmem->next == NULL){
            return true;
        } else return write_lmem(stream, lmem->next);
    }
}

bool serialize_lmem(FILE *stream, const srsvm_program *program)
{
    bool success = false;
    
    if(fwrite(&program->num_lmem_segments, sizeof(program->num_lmem_segments), 1, stream) != 1){
        success = false;
    } else {
        success = write_lmem(stream, program->literal_memory);
    }

    return success;
}

bool write_const(FILE *stream, const srsvm_constant_specification *c)
{
    if(c == NULL){
        return true;
    } else if(fwrite(&c->const_slot, sizeof(c->const_slot), 1, stream) != 1){
        return false;
    } else {
        const srsvm_constant_value *cv = &c->const_val;

        if(fwrite(&cv->type, sizeof(cv->type), 1, stream) != 1){
            return false;
        } else {

            switch(cv->type){
#define LOADER(field,flag) \
                case flag: \
                           if(fwrite(&cv->field, sizeof(cv->field), 1, stream) != 1){ \
                               return false; \
                           } \
                break;

                LOADER(word, WORD);
                LOADER(ptr, PTR);
                LOADER(ptr_offset, PTR_OFFSET);
                LOADER(bit, BIT);
                LOADER(u8, U8);
                LOADER(i8, I8);
                LOADER(u16, U16);
                LOADER(i16, I16);
#if WORD_SIZE == 32 || WORD_SIZE == 64 || WORD_SIZE == 128
                LOADER(u32, U32);
                LOADER(i32, I32);
                LOADER(f32, F32);
#endif
#if WORD_SIZE == 64 || WORD_SIZE == 128
                LOADER(u64, U64);
                LOADER(i64, I64);
                LOADER(f64, F64);
#endif
#if WORD_SIZE == 128
                LOADER(u128, U128);
                LOADER(i128, I128);
#endif
                case STR:
                if(fwrite(&cv->str_len, sizeof(cv->str_len), 1, stream) != 1){
                    return false;
                } else if(fwrite(cv->str, sizeof(char), (size_t) cv->str_len, stream) != (size_t) cv->str_len){
                    return false;
                }
                break;

                default:
                return false;
            }
        }

        if(c->next == NULL){
            return true;
        } else return write_const(stream, c->next);
    }
}

bool serialize_constants(FILE *stream, const srsvm_program *program)
{
    bool success = false;

    if(fwrite(&program->num_constants, sizeof(program->num_constants), 1, stream) != 1){
        success = false;
    } else {
        success = write_const(stream, program->constants);
    }

    return success;
}


bool srsvm_program_serialize(const char* output_path, const srsvm_program* program)
{
    bool success = false;

    FILE *stream = fopen(output_path, "wb");

    if(stream != NULL){
        if(! serialize_metadata(stream, program)){
            dbg_puts("failed to serialize metadata");
            goto error_cleanup;
        } else if(! serialize_registers(stream, program)){
            dbg_puts("failed to serialize registers");
            goto error_cleanup;
        } else if(! serialize_vmem(stream, program)){
            dbg_puts("failed to serialize vmem");
            goto error_cleanup;
        } else if(! serialize_lmem(stream, program)){
            dbg_puts("failed to serialize lmem");
            goto error_cleanup;
        } else if(! serialize_constants(stream, program)){
            dbg_puts("failed to serialize constants");
            goto error_cleanup;
        }

        fclose(stream);

        success = true;
    }

    return success;

error_cleanup:
    if(stream != NULL){
        fclose(stream);
    }

    return false;
}

#endif

uint8_t srsvm_program_word_size(const char* program_path)
{
    uint8_t word_size = 0;

    srsvm_program *program = srsvm_program_deserialize(program_path);

    if(program != NULL){
        word_size = program->metadata->word_size;

        srsvm_program_free(program);
    }

    return word_size;
}