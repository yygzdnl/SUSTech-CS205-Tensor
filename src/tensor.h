#include <variant>
#include <iostream>
#include <type_traits>
#include <iomanip>
#include <vector>
#include <memory>
#include "random"

namespace ts{
    using VariantData = std::variant<bool, int, float, double>;
    using std::vector;

    template<class T>
    int dtypeFrom(T t){
        if(typeid(T) == typeid(bool)) return 0;
        if(typeid(T) == typeid(int)) return 1;
        if(typeid(T) == typeid(float)) return 2;
        if(typeid(T) == typeid(double)) return 3;
    }

    class Tensor{
    private:
        std::shared_ptr<VariantData[]> data_shared;
        VariantData *data;

        int dimension;
        std::shared_ptr<int[]> shape;

        int total_size;
        int dtype;


        template<typename ArrayType, size_t N>
        void copyData(ArrayType(&arr)[N], VariantData *&dest, VariantData *destEnd){
            for(size_t i = 0; i < N; ++i){
                if constexpr(std::is_array<ArrayType>::value){
                    copyData(arr[i], dest, destEnd);
                }
                else{
                    *dest = arr[i];
                    if(dest < destEnd)
                        dest++;
                }
            }
        }

        template<typename ArrayType, size_t N>
        void getShape(ArrayType(&arr)[N], int dim){
            shape.get()[dim] = N;
            if constexpr(std::is_array<ArrayType>::value)
                getShape(arr[0], dim + 1);
        }


        void print(std::ostream &os, int index, int dim) const;


    public:
        static VariantData copy_tile(VariantData *src, Tensor *dst, int idx, int *src_shape, int dim);

        template<size_t N>
        static Tensor getShapeTensor(int(&size)[N]){
            Tensor t = Tensor();
            t.dimension = N;
            t.shape.reset(new int[t.dimension]);
            t.total_size = 1;
            for(int i = 0; i < t.dimension; i++){
                t.shape[i] = size[i];
                t.total_size *= size[i];
            }
            t.data_shared.reset(new VariantData[t.total_size]);
            t.data = t.data_shared.get();
            return t;
        }

        static Tensor getShapeTensor(const int size[], int N){
            Tensor t = Tensor();
            t.dimension = N;
            t.shape.reset(new int[t.dimension]);
            t.total_size = 1;
            for(int i = 0; i < t.dimension; i++){
                t.shape[i] = size[i];
                t.total_size *= size[i];
            }
            t.data_shared.reset(new VariantData[t.total_size]);
            t.data = t.data_shared.get();
            return t;
        }

        int *size();

        std::string type();

        VariantData *data_ptr();

        int get_total_size();

        int *get_shape() const;

        int get_dimension() const;

        void showShape(){
            for(int i = 0; i < dimension; i++){
                std::cout << shape[i] << " ";
            }
            std::cout << std::endl;
        }


        // Constructors
        // - Init tensor with exact data.
        template<typename T, size_t N>
        explicit Tensor(T(&arr)[N]){
            // dimension of arr, e.g. double[2][1] dimension = 2
            dimension = std::rank<T>::value + 1;
            shape.reset(new int[dimension]);
            using BaseType = typename std::remove_all_extents<T>::type();
            // type_name = typeid(BaseType).name();

            dtype = dtypeFrom(arr[0]);

            getShape(arr, 0);

            // copy array to data
            total_size = 1;
            for(int i = 0; i < dimension; i++){
                total_size *= shape[i];
            }
            data_shared.reset(new VariantData[total_size]);
            data = data_shared.get();
            VariantData *pointer = data;
            copyData(arr, pointer, data_shared.get() + total_size);
        }

        Tensor();

        int cal_stride(int dim, int *shape);

        friend std::ostream &operator<<(std::ostream &os, const Tensor &t);

        template<typename T, size_t N>
        static Tensor eye(int(&size)[N]){
            Tensor t = getShapeTensor(size);
            t.data_shared.reset(new VariantData[t.total_size]);
            t.data = t.data_shared.get();
            for(int i = 0; i < t.total_size; i++){
                t.data[i] = (T)0;
            }
            int min = t.shape[0] < t.shape[1] ? t.shape[0] : t.shape[1];
            for(int i = 0; i < min; i++){
                t.data[i * t.shape[1] + i] = (T)1;
            }
            return t;
        }

        Tensor operator()(int idx);
        Tensor operator()(int idx, std::pair<int, int> range);

        void operator=(const VariantData &value);

        template<size_t N>
        void operator=(const int(&arr)[N]){
            copyData(arr, data, data + total_size);
        }

        Tensor transpose(int dim1, int dim2);
        Tensor permute(int dim[]);

        template<size_t N>
        Tensor view(int(&shape)[N]){
            Tensor t = Tensor();
            t.data = data;
            t.data_shared = data_shared;
            t.total_size = total_size;
            t.dimension = N;
            t.shape.reset(new int[t.dimension]);
            for(int i = 0; i < t.dimension; i++){
                t.shape[i] = shape[i];
            }
            return t;
        }

        // Math operators
        static Tensor add(Tensor &t1, Tensor &t2);
        friend Tensor operator+(Tensor &t1, Tensor &t2);

        static Tensor sub(Tensor &t1, Tensor &t2);
        friend Tensor operator-(Tensor &t1, Tensor &t2);

        static Tensor mul(Tensor &t1, Tensor &t2);
        friend Tensor operator*(Tensor &t1, Tensor &t2);

        static Tensor div(Tensor &t1, Tensor &t2);

        static Tensor dot(Tensor &t1, Tensor &t2);

        // Reduction operators
        // Shrink the zero dims of a tensor.
        static Tensor shrink(Tensor &ts){

        }

        static Tensor sum(Tensor &ts, vector<int> dims){
            for(int dim : dims){
                if(dim < 0 || dim >= ts.get_dimension()){
                    throw std::invalid_argument("Invalid dimension input.");
                }
            }

            Tensor t = Tensor();
            t.shape.reset(new int[ts.get_dimension()]);

            std::sort(dims.begin(), dims.end());

            int j = 0;
            for(int i = 0; i < ts.dimension; i++){
                if(j < dims.size()){
                    // Need to be added.
                    if(i == dims[j]){
                        j++;
                    }
                }
                else{
                    break;
                }
            }

        }
    };



    template<typename T, size_t N>
    static Tensor rand(int(&size)[N]){
        Tensor t = Tensor::getShapeTensor(size);
        std::random_device rd;  // 获取随机数种子
        std::mt19937 gen(rd()); // 初始化Mersenne Twister伪随机数生成器
        std::uniform_real_distribution<> distrib(0, 100);

        for(int i = 0; i < t.get_total_size(); i++){
            t.data_ptr()[i] = (T)distrib(gen);
        }
        return t;
    }

    template<typename T, size_t N>
    static Tensor zeros(int(&size)[N]){
        Tensor t = Tensor::getShapeTensor(size);
        for(int i = 0; i < t.get_total_size(); i++){
            t.data_ptr()[i] = (T)0;
        }
        return t;
    }

    static Tensor zeros_like(Tensor &ts){
        int *arr = ts.get_shape();
        const int len = sizeof(arr) / sizeof(arr[0]);
        int shape_arr[len];
        Tensor result = zeros<int>(shape_arr);
        return result;
    }

    template<typename T, size_t N>
    static Tensor ones(int(&size)[N]){
        Tensor t = Tensor::getShapeTensor(size);
        for(int i = 0; i < t.get_total_size(); i++){
            t.data_ptr()[i] = (T)1;
        }
        return t;
    }

    template<typename T, size_t N>
    static Tensor full(int(&size)[N], T value){
        Tensor t = Tensor::getShapeTensor(size);
        for(int i = 0; i < t.get_total_size(); i++){
            t.data_ptr()[i] = (T)value;
        }
        return t;
    }

    static Tensor cat(const std::pair<Tensor, Tensor> &tensors, int dim);

    template<size_t N>
    static Tensor tile(Tensor &tensor, int(&dims)[N]){
        const int dimension = tensor.get_dimension();
        if(dimension != N)
            throw std::invalid_argument("dimension of tensor and dims should be equal");
        int size[dimension];
        for(int i = 0; i < dimension; i++){
            size[i] = tensor.get_shape()[i] * dims[i];
            std::cout << size[i] << std::endl;
        }
        Tensor t = Tensor::getShapeTensor(size, dimension);

        for(int i = 0; i < t.get_total_size(); i++){
            t.data_ptr()[i] = Tensor::copy_tile((tensor.data_ptr()), &t, i, tensor.get_shape(), 0);
        }
        return t;
    }

    static Tensor transpose(Tensor tensor, int dim1, int dim2);
    static Tensor permute(Tensor tensor, int dim[]);

    template<size_t N>
    static Tensor view(Tensor tensor, int(&shape)[N]){
        return tensor.view(shape);
    }
}

