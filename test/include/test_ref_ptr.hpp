
#include <ref_ptr.hpp>

class tmp
{
public:
    explicit tmp(const char* value_)
        : value {value_}
    {
    }

    void Print(const char* name) noexcept
    {
        printf("%s : %s\n", name, value);
    }

private:
    const char* value;
};

int test_ref_ptr()
{
    tmp normal_tmp("normal");
    tmp normal_tmp_for_ref_ptr("ref_ptr");
    ref_ptr<tmp> ref_ptr_tmp(normal_tmp_for_ref_ptr);
    auto unique_ptr_tmp = std::make_unique<tmp>("unique_ptr");

    ref_ptr<tmp> test1(normal_tmp);
    ref_ptr<tmp> test2(ref_ptr_tmp);
    ref_ptr<tmp> test3(unique_ptr_tmp);
    ref_ptr<tmp> test4; test4 = normal_tmp;
    ref_ptr<tmp> test5; test5 = ref_ptr_tmp;
    ref_ptr<tmp> test6; test6 = unique_ptr_tmp;
    
    test1->Print("test1");
    test2->Print("test2");
    test3->Print("test3");
    (*test4).Print("test4");
    (*test5).Print("test5");
    (*test6).Print("test6");

    return 0;
}

