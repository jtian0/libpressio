#include <libpressio_ext/cpp/data.h>
#include <libpressio_ext/cpp/domain.h>
#include <libpressio_ext/cpp/domain_send.h>
#include <libpressio_ext/cpp/registry.h>
#include <cuda_runtime.h>
/**
 * \file
 * \brief domain for cudaMalloc/cudaFree
 */
struct pressio_cudamalloc_domain: public pressio_domain {
    void* alloc(size_t n) override {
        void* ptr = nullptr;
        auto err = cudaMalloc(&ptr, n);
        if(err != cudaSuccess) {
            throw std::bad_alloc();
        }
        return ptr;
    }
    void free(void* data, size_t) override {
        if(data) {
            cudaFree(data);
            data = nullptr;
        }
    }
    void memcpy(void* dst, void* src, size_t n) override {
        cudaMemcpy(dst, src, n, cudaMemcpyDeviceToDevice);
    }
    bool equal(pressio_domain const& rhs) const noexcept override {
        return dynamic_cast<pressio_cudamalloc_domain const*>(&rhs) != nullptr;
    }
    std::string const& prefix() const override {
        static const std::string pfx = "cudamalloc";
        return pfx;
    }
    std::shared_ptr<pressio_domain> clone() override {
        return std::make_shared<pressio_cudamalloc_domain>(*this);
    }
};

struct pressio_domain_send_host_to_device: public pressio_domain_send {
    void send(pressio_data& dst, pressio_data const& src) const override {
        auto err = cudaMemcpy(dst.data(), src.data(), dst.size_in_bytes(), cudaMemcpyHostToDevice);
        if(err != cudaSuccess) {
            throw std::runtime_error(cudaGetErrorString(err));
        }
    }
};
struct pressio_domain_send_device_to_host: public pressio_domain_send {
    void send(pressio_data& dst, pressio_data const& src) const override {
        auto err = cudaMemcpy(dst.data(), src.data(), dst.size_in_bytes(), cudaMemcpyDeviceToHost);
        if(err != cudaSuccess) {
            throw std::runtime_error(cudaGetErrorString(err));
        }
    }
};

pressio_register cudamalloc_register(domain_plugins(), "cudamalloc", []{return std::make_shared<pressio_cudamalloc_domain>();});
pressio_register cudamalloc_to_malloc(domain_send_plugins(), "cudamalloc>malloc", []{return std::make_unique<pressio_domain_send_device_to_host>();});
pressio_register malloc_to_cudamalloc(domain_send_plugins(), "malloc>cudamalloc", []{return std::make_unique<pressio_domain_send_host_to_device>();});
pressio_register cudamallochost_to_cudamalloc(domain_send_plugins(), "cudamallochost>cudamalloc", []{return std::make_unique<pressio_domain_send_device_to_host>();});
pressio_register cudamalloc_to_cudamallochost(domain_send_plugins(), "cudamalloc>cudamallochost", []{return std::make_unique<pressio_domain_send_host_to_device>();});
