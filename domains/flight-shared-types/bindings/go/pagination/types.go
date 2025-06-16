// Package paginationtypes provides Go bindings for Flight Pagination Types
package paginationtypes

// ListRequest represents a paginated list request
type ListRequest struct {
	Page    uint32   `json:"page"`
	PerPage uint32   `json:"per_page"`
	Filters []string `json:"filters"`
}

// ListResponse represents a paginated list response
type ListResponse struct {
	Items       []string `json:"items"`
	TotalCount  uint32   `json:"total_count"`
	Page        uint32   `json:"page"`
	PerPage     uint32   `json:"per_page"`
	HasNext     bool     `json:"has_next"`
	HasPrevious bool     `json:"has_previous"`
}

// PaginationConfig contains pagination configuration
type PaginationConfig struct {
	DefaultPerPage uint32 `json:"default_per_page"`
	MaxPerPage     uint32 `json:"max_per_page"`
	MaxPage        uint32 `json:"max_page"`
}

// NewListRequest creates a new list request with default values
func NewListRequest(page, perPage uint32) ListRequest {
	return ListRequest{
		Page:    page,
		PerPage: perPage,
		Filters: make([]string, 0),
	}
}

// AddFilter adds a filter to the list request
func (lr *ListRequest) AddFilter(filter string) {
	lr.Filters = append(lr.Filters, filter)
}

// Validate validates the list request parameters
func (lr *ListRequest) Validate(config PaginationConfig) error {
	if lr.Page == 0 {
		lr.Page = 1
	}
	if lr.PerPage == 0 {
		lr.PerPage = config.DefaultPerPage
	}
	if lr.PerPage > config.MaxPerPage {
		lr.PerPage = config.MaxPerPage
	}
	if lr.Page > config.MaxPage {
		lr.Page = config.MaxPage
	}
	return nil
}

// CalculateOffset calculates the offset for database queries
func (lr *ListRequest) CalculateOffset() uint32 {
	return (lr.Page - 1) * lr.PerPage
}

// NewListResponse creates a paginated response
func NewListResponse(items []string, totalCount, page, perPage uint32) ListResponse {
	totalPages := (totalCount + perPage - 1) / perPage

	return ListResponse{
		Items:       items,
		TotalCount:  totalCount,
		Page:        page,
		PerPage:     perPage,
		HasNext:     page < totalPages,
		HasPrevious: page > 1,
	}
}

// GetTotalPages calculates the total number of pages
func (lr *ListResponse) GetTotalPages() uint32 {
	return (lr.TotalCount + lr.PerPage - 1) / lr.PerPage
}

// GetNextPage returns the next page number if available
func (lr *ListResponse) GetNextPage() *uint32 {
	if lr.HasNext {
		next := lr.Page + 1
		return &next
	}
	return nil
}

// GetPreviousPage returns the previous page number if available
func (lr *ListResponse) GetPreviousPage() *uint32 {
	if lr.HasPrevious {
		prev := lr.Page - 1
		return &prev
	}
	return nil
}

// DefaultPaginationConfig returns default pagination configuration
func DefaultPaginationConfig() PaginationConfig {
	return PaginationConfig{
		DefaultPerPage: 20,
		MaxPerPage:     100,
		MaxPage:        1000,
	}
}

// V6RPaginationConfig returns V6R-optimized pagination configuration
func V6RPaginationConfig() PaginationConfig {
	return PaginationConfig{
		DefaultPerPage: 10,
		MaxPerPage:     50,
		MaxPage:        500,
	}
}

// DreamcastPaginationConfig returns Dreamcast-optimized pagination configuration
func DreamcastPaginationConfig() PaginationConfig {
	return PaginationConfig{
		DefaultPerPage: 5,
		MaxPerPage:     20,
		MaxPage:        100,
	}
}

// ListManager manages paginated lists
type ListManager struct {
	config PaginationConfig
}

// NewListManager creates a new list manager
func NewListManager(config PaginationConfig) *ListManager {
	return &ListManager{
		config: config,
	}
}

// ProcessRequest processes a list request and returns a response
func (lm *ListManager) ProcessRequest(request ListRequest, allItems []string) (ListResponse, error) {
	err := request.Validate(lm.config)
	if err != nil {
		return ListResponse{}, err
	}

	// Apply filters if any
	filteredItems := lm.applyFilters(allItems, request.Filters)

	// Calculate pagination
	totalCount := uint32(len(filteredItems))
	offset := request.CalculateOffset()

	// Get items for current page
	var pageItems []string
	if offset < totalCount {
		end := offset + request.PerPage
		if end > totalCount {
			end = totalCount
		}
		pageItems = filteredItems[offset:end]
	} else {
		pageItems = make([]string, 0)
	}

	return NewListResponse(pageItems, totalCount, request.Page, request.PerPage), nil
}

// applyFilters applies filters to the items list
func (lm *ListManager) applyFilters(items []string, filters []string) []string {
	if len(filters) == 0 {
		return items
	}

	var filtered []string
	for _, item := range items {
		include := true
		for _, filter := range filters {
			// Simple contains filter - could be enhanced with regex
			if filter != "" && !contains(item, filter) {
				include = false
				break
			}
		}
		if include {
			filtered = append(filtered, item)
		}
	}
	return filtered
}

// contains checks if a string contains a substring
func contains(str, substr string) bool {
	return len(substr) == 0 || len(str) >= len(substr) &&
		(str == substr || len(str) > len(substr) &&
			indexOfString(str, substr) >= 0)
}

// indexOfString finds the index of a substring in a string
func indexOfString(str, substr string) int {
	for i := 0; i <= len(str)-len(substr); i++ {
		if str[i:i+len(substr)] == substr {
			return i
		}
	}
	return -1
}
